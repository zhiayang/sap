// tag.h
// Copyright (c) 2021, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace font
{
	struct Tag
	{
		constexpr Tag() : value(0) { }
		constexpr explicit Tag(uint32_t sp) : value(sp) { }
		constexpr explicit Tag(const char (&f)[5]) : Tag((uint8_t) f[0], (uint8_t) f[1], (uint8_t) f[2], (uint8_t) f[3])
		{
		}

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

		inline size_t hash() const { return std::hash<uint32_t>()(value); }

		constexpr bool operator<(const Tag& t) const { return this->value < t.value; }

		uint32_t value;
	};
}

template <>
struct std::hash<font::Tag>
{
	size_t operator()(font::Tag t) const { return t.hash(); }
};
