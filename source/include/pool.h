// pool.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>

#include "error.h"

namespace util
{
	template <typename T, size_t REGION_SIZE = (1 << 12)>
	struct Pool
	{
		using value_type = T;
		static_assert(REGION_SIZE >= sizeof(T));

		struct Region
		{
			Region(size_t capacity, Region* next)
			{
				m_next = next;
				m_consumed = 0;
				m_capacity = capacity;

				auto alignment = alignof(T);
				if(alignment < alignof(void*))
					alignment = alignof(void*);

				// TODO: report a proper error
				void* ptr = 0;
				if(posix_memalign(&ptr, alignment, m_capacity) != 0)
					sap::internal_error("out of memory (trying to allocate {} bytes with {}-byte alignment)", capacity,
					    alignment);

				m_memory = reinterpret_cast<uint8_t*>(ptr);
				assert(m_memory != nullptr);
			}

			~Region()
			{
				free(m_memory);
				if(m_next)
					delete m_next;

				m_next = nullptr;
				m_memory = nullptr;
			}

			Region* m_next = 0;
			uint8_t* m_memory = 0;

			size_t m_consumed = 0;
			size_t m_capacity = 0;
		};


		template <typename... Args>
		T* allocate(Args&&... args)
		{
			if(!this->region)
				this->region = new Region(REGION_SIZE, nullptr);

			assert(this->region != nullptr);
			auto head = this->region;

			if(head->m_capacity - head->m_consumed < sizeof(T))
			{
				this->region = new Region(REGION_SIZE, head);
				head = this->region;
			}

			assert(head->m_capacity - head->m_consumed >= sizeof(T));
			auto mem = head->m_memory + head->m_consumed;
			head->m_consumed += sizeof(T);

			return new(mem) T(static_cast<Args&&>(args)...);
		}

		void* allocate_raw(size_t bytes)
		{
			if(!this->region)
				this->region = new Region(REGION_SIZE, nullptr);

			assert(this->region != nullptr);
			auto head = this->region;

			if(head->m_capacity - head->m_consumed < bytes)
			{
				this->region = new Region(REGION_SIZE, head);
				head = this->region;
			}

			assert(head->m_capacity - head->m_consumed >= bytes);
			auto mem = head->m_memory + head->m_consumed;
			head->m_consumed += bytes;

			return mem;
		}

	private:
		Region* region = 0;
	};

	template <typename T, typename... Args>
	T* make(Args&&... args)
	{
		static Pool<T> pool;
		return pool.allocate(static_cast<Args&&>(args)...);
	}

	template <typename T>
	struct pool_allocator
	{
		using value_type = T;

		T* allocate(size_t n) { return static_cast<T*>(s_pool.allocate_raw(n * sizeof(T))); }
		void deallocate(T* ptr, size_t n)
		{
			(void) ptr;
			(void) n;
		}

	private:
		static inline Pool<T> s_pool;
	};
}
