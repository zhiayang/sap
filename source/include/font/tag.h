// tag.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <cstdint>
#include <cstddef>

namespace font
{
	struct Tag
	{
		constexpr Tag() : be_value(0) { }
		constexpr explicit Tag(uint32_t sp) : be_value(sp) { }
		constexpr explicit Tag(const char (&f)[5]) : Tag(f[0], f[1], f[2], f[3]) { }

		constexpr Tag(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
			: be_value(((uint32_t) a << 24) | ((uint32_t) b << 16) | ((uint32_t) c << 8) | ((uint32_t) d << 0))
		{ }

		constexpr bool operator== (const Tag& t) const { return this->be_value == t.be_value; }
		constexpr bool operator!= (const Tag& t) const { return !(*this == t); };

		inline std::string str() const
		{
			return std::string( {
				(char) ((be_value & 0xff000000) >> 24),
				(char) ((be_value & 0x00ff0000) >> 16),
				(char) ((be_value & 0x0000ff00) >> 8),
				(char) ((be_value & 0x000000ff) >> 0),
			});
		}

		constexpr bool operator< (const Tag& t) const { return this->be_value < t.be_value; }

		uint32_t be_value;
	};
}
