// tag.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <cstddef>

namespace font
{
	struct Tag
	{
		constexpr Tag() : value(0) { }
		constexpr explicit Tag(uint32_t sp) : value(sp) { }
		constexpr explicit Tag(const char (&f)[5]) : Tag(f[0], f[1], f[2], f[3]) { }

		constexpr Tag(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
			: value(((uint32_t) a << 24) | ((uint32_t) b << 16) | ((uint32_t) c << 8) | ((uint32_t) d << 0))
		{
		}

		constexpr bool operator==(const Tag& t) const { return this->value == t.value; }
		constexpr bool operator!=(const Tag& t) const { return !(*this == t); };

		inline std::string str() const
		{
			return std::string({
				(char) ((value & 0xff000000) >> 24),
				(char) ((value & 0x00ff0000) >> 16),
				(char) ((value & 0x0000ff00) >> 8),
				(char) ((value & 0x000000ff) >> 0),
			});
		}

		constexpr bool operator<(const Tag& t) const { return this->value < t.value; }

		uint32_t value;
	};
}
