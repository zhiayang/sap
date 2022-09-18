// gdef.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cassert>

#include "util.h"
#include "error.h"
#include "font/font.h"
#include "font/features.h"

namespace font::off
{
	int getGlyphClass(zst::byte_span table, GlyphId glyphId)
	{
		auto format = consume_u16(table);

		if(format != 1 && format != 2)
			sap::internal_error("invalid ClassDef format {}", format);

		auto gid16 = static_cast<uint16_t>(glyphId);
		if(format == 1)
		{
			auto start_gid = consume_u16(table);
			auto num_glyphs = consume_u16(table);

			// 0 is the default class.
			if(gid16 < start_gid || gid16 >= start_gid + num_glyphs)
				return 0;

			auto array = table.cast<uint16_t>();
			return util::convertBEU16(array[gid16 - start_gid]);
		}
		else if(format == 2)
		{
			auto num_ranges = consume_u16(table);

			struct ClassRangeRecord
			{
				uint16_t start_gid;
				uint16_t end_gid;
				uint16_t glyph_class;
			} __attribute__((packed));
			static_assert(sizeof(ClassRangeRecord) == 3 * sizeof(uint16_t));

			auto array = table.take(num_ranges * sizeof(ClassRangeRecord)).cast<ClassRangeRecord>();

			size_t low = 0;
			size_t high = num_ranges;
			while(low < high)
			{
				auto mid = (low + high) / 2u;
				auto rec = array[mid];

				auto start_gid = util::convertBEU16(rec.start_gid);
				auto end_gid = util::convertBEU16(rec.end_gid);

				if(start_gid <= gid16 && gid16 <= end_gid)
					return util::convertBEU16(rec.glyph_class);
				else if(end_gid < gid16)
					low = mid + 1;
				else
					high = mid;
			}

			// again, 0 is the default.
			return 0;
		}

		assert(false);
	}


	template <typename Callback>
	static void parse_classdef_table(zst::byte_span table, Callback&& callback)
	{
		auto format = consume_u16(table);

		if(format != 1 && format != 2)
			sap::internal_error("invalid ClassDef format {}", format);

		if(format == 1)
		{
			auto start_gid = consume_u16(table);
			auto num_glyphs = consume_u16(table);

			for(uint32_t i = 0; i < num_glyphs; i++)
				callback(consume_u16(table), GlyphId { start_gid + i });
		}
		else
		{
			auto num_ranges = consume_u16(table);

			for(size_t i = 0; i < num_ranges; i++)
			{
				auto first_gid = consume_u16(table);
				auto last_gid = consume_u16(table);
				auto class_id = consume_u16(table);

				for(auto g = first_gid; g <= last_gid; g++)
					callback(class_id, GlyphId { g });
			}
		}
	}


	std::map<int, std::set<GlyphId>> parseAllClassDefs(zst::byte_span table)
	{
		std::map<int, std::set<GlyphId>> class_ids;
		parse_classdef_table(table, [&](int cls, GlyphId gid) {
			class_ids[cls].insert(gid);
		});

		return class_ids;
	}

	std::map<GlyphId, int> parseGlyphToClassMapping(zst::byte_span table)
	{
		std::map<GlyphId, int> class_ids;
		parse_classdef_table(table, [&](int cls, GlyphId gid) {
			class_ids[gid] = cls;
		});

		return class_ids;
	}
}
