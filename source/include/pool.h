// pool.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <cassert>
#include <cstdlib>
#include <cstddef>

namespace util
{
	template <typename T>
	struct Pool
	{
		using value_type = T;

		static constexpr size_t REGION_SIZE = 1 << 12;
		static_assert(REGION_SIZE >= sizeof(T));

		struct Region
		{
			Region(size_t capacity, Region* next)
			{
				this->next = next;
				this->consumed = 0;
				this->capacity = capacity;

				// TODO: report a proper error
				this->memory = reinterpret_cast<uint8_t*>(std::aligned_alloc(alignof(T), capacity));
				assert(this->memory != nullptr);
			}

			~Region()
			{
				std::free(this->memory);
				if(this->next)
					delete this->next;

				this->next = nullptr;
				this->memory = nullptr;
			}

			Region* next = 0;
			uint8_t* memory = 0;

			size_t consumed = 0;
			size_t capacity = 0;
		};


		template <typename... Args>
		T* allocate(Args&&... args)
		{
			if(!this->region)
				this->region = new Region(REGION_SIZE, nullptr);

			assert(this->region != nullptr);
			auto head = this->region;

			if(head->capacity - head->consumed < sizeof(T))
			{
				this->region = new Region(REGION_SIZE, head);
				head = this->region;
			}

			assert(head->capacity - head->consumed >= sizeof(T));
			auto mem = head->memory + head->consumed;
			head->consumed += sizeof(T);

			return new(mem) T(static_cast<Args&&>(args)...);
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
}
