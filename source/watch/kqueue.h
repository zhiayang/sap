// kqueue.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__DragonFly__) \
    || defined(__APPLE__)
#define SAP_HAVE_WATCH 1

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/event.h>

#include <pthread.h>

#include <list>
#include <mutex>
#include <chrono>
#include <vector>
#include <semaphore>

#include "defs.h"

namespace sap::watch
{
	struct Event
	{
		int fd;
		std::string path;
	};

	struct State
	{
		std::string main_file;
		std::string output_file;

		std::list<Event> event_contexts;
		std::vector<struct kevent> kevents;
		std::optional<pthread_t> current_compile_thread;

		std::chrono::steady_clock::time_point compile_start;

		std::recursive_mutex mutex;
		std::binary_semaphore ready_sem { 0 };

		void reset();
	};

	static State g_state;
	static std::optional<int> g_kqueue;

	StrErrorOr<void> addFileToWatchList(zst::str_view path)
	{
		if(not g_kqueue.has_value())
		{
			if(int kq = kqueue(); kq < 0)
				return ErrFmt("failed to create kqueue: {} ({})", strerror(errno), errno);
			else
				g_kqueue = kq;
		}

		assert(g_kqueue.has_value());

		int fd = open(path.str().c_str(), O_EVTONLY);
		if(fd < 0)
		{
			return ErrFmt("failed to open file '{}' for watching: {} ({})", //
			    path, strerror(errno), errno);
		}

		auto lk = std::unique_lock(g_state.mutex);
		auto& event = g_state.event_contexts.emplace_back(Event {
		    .fd = fd,
		    .path = path.str(),
		});

		auto& kevent = g_state.kevents.emplace_back();

		EV_SET(&kevent, fd, EVFILT_VNODE,                                       //
		    (EV_ADD | EV_CLEAR | EV_ONESHOT),                                   //
		    (NOTE_DELETE | NOTE_WRITE | NOTE_RENAME | NOTE_LINK | NOTE_REVOKE), //
		    0, &event);

		return Ok();
	}

	void State::reset()
	{
		for(auto& c : this->event_contexts)
			close(c.fd);

		this->kevents.clear();
		this->event_contexts.clear();
	}

	bool isWatching()
	{
		// decent proxy.
		return not g_state.main_file.empty();
	}

	static void* compile_runner(void*)
	{
		pthread_detach(pthread_self());
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
		pthread_cleanup_push(
		    [](void*) {
			    auto elapsed = std::chrono::steady_clock::now() - g_state.compile_start;
			    auto ms_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

			    // while we have the mutex, reset the compile thread (since we're going away)
			    g_state.current_compile_thread.reset();
			    zpr::println("-----------------------------");
			    zpr::println("compilation took {.2f} seconds\n", static_cast<double>(ms_elapsed) / 1000.0);

			    g_state.mutex.unlock();
		    },
		    0);

		// lock the mutex, then tell the main thread that we're ready
		g_state.mutex.lock();
		g_state.current_compile_thread = pthread_self();

		g_state.ready_sem.release();

		addFileToWatchList(g_state.main_file);

		g_state.compile_start = std::chrono::steady_clock::now();
		(void) sap::compile(g_state.main_file, g_state.output_file);

		pthread_cleanup_pop(1);
		return nullptr;
	}

	void start(zst::str_view main_file, zst::str_view output_file)
	{
		if(not g_kqueue.has_value())
			return;

		g_state.main_file = main_file.str();
		g_state.output_file = output_file.str();

		// compile once
		(void) sap::compile(main_file, output_file);

		constexpr static size_t NUM_EVENTS = 16;
		auto new_events = new struct kevent[NUM_EVENTS];

		// wait indefinitely.
		while(true)
		{
			// acquire the lock; this implies that we must wait for existing compile jobs
			// to either finish or be terminated.
			auto lk = std::unique_lock(g_state.mutex);

			int num_events = kevent(*g_kqueue, g_state.kevents.data(), (int) g_state.kevents.size(), //
			    &new_events[0], NUM_EVENTS, nullptr);

			if(num_events < 0)
			{
				zpr::fprintln(stderr, "kevent error: {} ({})", strerror(errno), errno);
				break;
			}

			// by right this shouldn't happen, since we wait indefinitely.
			if(num_events == 0)
				continue;

			for(int i = 0; i < num_events; i++)
			{
				auto& event = new_events[i];

				if(event.flags & EV_ERROR)
				{
					zpr::fprintln(stderr, "kevent error for '{}': {} ({})", //
					    static_cast<Event*>(event.udata)->path,             //
					    strerror(static_cast<int>(event.data)), event.data);

					continue;
				}

				// stop looking at the rest of the events, just start recompiling.
				util::log("file '{}' changed, recompiling", static_cast<Event*>(event.udata)->path);

				// if there is already a thread running, just kill it.
				if(g_state.current_compile_thread.has_value())
				{
					util::log("cancelling current compile job");
					pthread_cancel(*g_state.current_compile_thread);
				}

				// clear the things we are watching (compilation will re-add them as
				// we re-discover files)
				g_state.reset();

				pthread_t thr {};
				pthread_create(&thr, nullptr, &compile_runner, nullptr);

				// unlock the mutex so the compile job can proceed
				lk.unlock();

				// before breaking (and as a result re-acquiring the mutex), wait
				// for the worker thread to signal that it's ready. this prevents us from
				// always being the one to acquire the mutex.
				g_state.ready_sem.acquire();
				break;
			}
		}

		delete[] new_events;
	}
}

#endif
