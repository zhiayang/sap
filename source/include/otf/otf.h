// otf.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace otf
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

		std::string str() const
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

	struct Table
	{
		Tag tag;
		uint32_t offset;
		uint32_t length;
		uint32_t checksum;
	};

	struct OTFont
	{
		static OTFont* parseFromFile(const std::string& path);

		// corresponds to name IDs 16 and 17. if not present, they will have the same
		// value as their *_compat counterparts.
		std::string family;
		std::string subfamily;

		// corresponds to name IDs 1 and 2.
		std::string family_compat;
		std::string subfamily_compat;

		std::string unique_name;        // name 3

		std::string full_name;          // name 4
		std::string postscript_name;    // name 6

		// when embedding this font, we are probably legally required to reproduce
		// the license.
		std::string copyright_info;     // name 0
		std::string license_info;       // name 13

		int font_type = 0;
		std::map<Tag, Table> tables;

		uint8_t* file_bytes;
		size_t file_size;

		static constexpr int TYPE_TRUETYPE = 1;
		static constexpr int TYPE_CFF      = 2;
	};
}
