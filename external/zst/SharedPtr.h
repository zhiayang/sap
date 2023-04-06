// SharedPtr.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <stdint.h>
#include <stddef.h>

#include <atomic>
#include <type_traits>

namespace zst
{
	template <typename T, bool AtomicRefCount = false>
	struct IntrusiveRefCounted
	{
		IntrusiveRefCounted() { }

		IntrusiveRefCounted(IntrusiveRefCounted&&) = delete;
		IntrusiveRefCounted(const IntrusiveRefCounted&&) = delete;
		IntrusiveRefCounted& operator=(IntrusiveRefCounted&&) = delete;
		IntrusiveRefCounted& operator=(const IntrusiveRefCounted&&) = delete;

		void retain() const
		{
			if constexpr(AtomicRefCount)
				m_refcount.fetch_add(1, std::memory_order::relaxed);
			else
				m_refcount += 1;
		}

		bool release() const
		{
			auto old_rc = [&]() {
				if constexpr(AtomicRefCount)
					return m_refcount.fetch_sub(1, std::memory_order::acq_rel);
				else
					return m_refcount--;
			}();

			// if the old refcount was 1, it's time to destroy this guy
			return old_rc == 1;
		}

		size_t refcount() const
		{
			if constexpr(AtomicRefCount)
				return m_refcount.load(std::memory_order::relaxed);
			else
				return m_refcount;
		}

	private:
		mutable std::conditional_t<AtomicRefCount, std::atomic<size_t>, size_t> m_refcount = 1;
	};

	template <typename T>
	struct SharedPtr
	{
		using element_type = T;

	private:
		struct adopt_tag
		{
		};

	public:
		SharedPtr() : m_ptr(nullptr) { }
		~SharedPtr() { this->clear(); }

		SharedPtr([[maybe_unused]] std::nullptr_t _) : m_ptr(nullptr) { }
		SharedPtr([[maybe_unused]] adopt_tag _, T* ptr) : m_ptr(const_cast<T*>(ptr)) { }

		explicit SharedPtr(const T* ptr) : m_ptr(const_cast<T*>(ptr)) { this->retain_ptr(); }

		template <typename U>
		explicit SharedPtr(const U* ptr)
			requires(std::is_convertible_v<U*, T*>)
			: m_ptr(const_cast<U*>(ptr))
		{
			this->retain_ptr();
		}

		explicit SharedPtr(const T& ptr) : m_ptr(const_cast<T*>(&ptr)) { this->retain_ptr(); }

		template <typename U>
		explicit SharedPtr(const U& ptr)
			requires(std::is_convertible_v<U&, T&>)
			: m_ptr(&const_cast<U&>(ptr))
		{
			this->retain_ptr();
		}


		SharedPtr(SharedPtr&& other) : m_ptr(other.take_reference()) { }
		SharedPtr(const SharedPtr& other) : m_ptr(other.m_ptr) { this->retain_ptr(); }

		template <typename U>
		SharedPtr(const SharedPtr<U>& other)
			requires(std::is_convertible_v<U*, T*>)
			: m_ptr(other.m_ptr)
		{
			this->retain_ptr();
		}

		template <typename U>
		SharedPtr(SharedPtr<U>&& other)
			requires(std::is_convertible_v<U*, T*>)
			: m_ptr(other.take_reference())
		{
		}

		SharedPtr& operator=(const SharedPtr& other)
		{
			SharedPtr tmp = other;
			this->swap(tmp);
			return *this;
		}

		SharedPtr& operator=(SharedPtr&& other)
		{
			auto tmp = SharedPtr(std::move(other));
			this->swap(tmp);
			return *this;
		}


		template <typename U>
		SharedPtr& operator=(const SharedPtr<U>& other)
			requires(std::is_convertible_v<U*, T*>)
		{
			SharedPtr tmp = other;
			this->swap(tmp);
			return *this;
		}

		template <typename U>
		SharedPtr& operator=(SharedPtr<U>&& other)
			requires(std::is_convertible_v<U*, T*>)
		{
			auto tmp = SharedPtr(std::move(other));
			this->swap(tmp);
			return *this;
		}

		SharedPtr& operator=([[maybe_unused]] std::nullptr_t n)
		{
			this->clear();
			return *this;
		}

		void reset(T* ptr)
		{
			this->clear();
			m_ptr = ptr;
		}

		void swap(SharedPtr& other)
		{
			using std::swap;
			swap(m_ptr, other.m_ptr);
		}

		[[nodiscard]] T* leak_reference()
		{
			this->retain_ptr();
			return m_ptr;
		}

		operator bool() const { return m_ptr != nullptr; }
		bool operator==(const T* ptr) const { return m_ptr == ptr; }
		bool operator==(const SharedPtr& other) const { return m_ptr == other.m_ptr; }
		bool operator==([[maybe_unused]] std::nullptr_t _) const { return m_ptr == nullptr; }

		void clear()
		{
			this->release_ptr();
			m_ptr = nullptr;
		}

		[[nodiscard]] T* take_reference() { return std::exchange(m_ptr, nullptr); }

		size_t refcount() const
		{
			if(m_ptr == nullptr)
				return 0;

			return m_ptr->refcount();
		}

		size_t use_count() const { return this->refcount(); }

		// for some reason, std::shared_ptr always gives you a non-const pointer.
		// i guess you need to explicitly name shared_ptr<const T> if you want constness.

		T* operator->() const { return this->get(); }
		T& operator*() const { return *this->get(); }

		[[nodiscard]] T* get() const { return m_ptr; }

		template <typename... Args>
		[[nodiscard]] static SharedPtr make(Args&&... args)
		{
			return SharedPtr(adopt_tag(), new T(static_cast<Args&&>(args)...));
		}

		[[nodiscard]] static SharedPtr adopt(T* ptr) { return SharedPtr(adopt_tag(), ptr); }

	private:
		void retain_ptr() const
		{
			if(m_ptr == nullptr)
				return;

			m_ptr->retain();
		}

		void release_ptr() const
		{
			if(m_ptr == nullptr)
				return;

			if(m_ptr->release())
				delete m_ptr;
		}

		template <typename>
		friend struct SharedPtr;

	private:
		T* m_ptr;
	};

	template <typename T>
	SharedPtr(const T*) -> SharedPtr<T>;

	template <typename T>
	SharedPtr(T*) -> SharedPtr<T>;

	template <typename T, typename... Args>
	SharedPtr<T> make_shared(Args&&... args)
	{
		return SharedPtr<T>::make(static_cast<Args&&>(args)...);
	}
}
