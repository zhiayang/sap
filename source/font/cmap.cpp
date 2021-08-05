// cmap.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <zst.h>
#include <cassert>
#include <cstring>

#include "util.h"
#include "font/font.h"

namespace font
{
	extern uint16_t peek_u16(const zst::byte_span& s);
	extern uint32_t peek_u32(const zst::byte_span& s);
	extern uint8_t consume_u8(zst::byte_span& s);
	extern uint16_t consume_u16(zst::byte_span& s);
	extern int16_t consume_i16(zst::byte_span& s);
	extern uint32_t consume_u24(zst::byte_span& s);
	extern uint32_t consume_u32(zst::byte_span& s);

	// all of these return 0 if the codepoint is not found, which should (in all sensible fonts)
	// correspond to the "notdef" character, which is usually a rectangle with an 'X' in it.
	static uint32_t find_in_subtable_0(zst::byte_span subtable, uint32_t codepoint)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 0);

		auto len = consume_u16(subtable);
		assert(len == 3 * sizeof(uint16_t) + 256 * sizeof(uint8_t));

		auto lang = consume_u16(subtable);
		(void) lang;

		if(codepoint <= 255) return subtable[codepoint];
		else                 return 0;
	}

	static uint32_t find_in_subtable_4(zst::byte_span subtable, uint32_t codepoint)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 4);

		if(codepoint > 0xffff)
			return 0;

		auto len = consume_u16(subtable);
		auto lang = consume_u16(subtable);

		(void) len;
		(void) lang;

		// what the fuck is this format? fmt 12 is so much better.
		auto seg_count = consume_u16(subtable) / 2u;
		consume_u16(subtable);  // search_range, ignored
		consume_u16(subtable);  // entry_selector, ignored
		consume_u16(subtable);  // range_shift, ignored

		auto u16_array = subtable.cast<uint16_t>();

		auto end_codes = u16_array.take_prefix(seg_count);
		u16_array.remove_prefix(1); // reserved

		auto start_codes = u16_array.take_prefix(seg_count);
		auto id_deltas = u16_array.take_prefix(seg_count);
		auto id_range_offsets = u16_array.take_prefix(seg_count);

		for(size_t i = 0; i < seg_count; i++)
		{
			auto start = util::convertBEU16(start_codes[i]);
			auto end = util::convertBEU16(end_codes[i]);
			auto delta = util::convertBEU16(id_deltas[i]);
			auto range_ofs = util::convertBEU16(id_range_offsets[i]);

			if(start <= codepoint && codepoint <= end)
			{
				if(range_ofs != 0)
				{
					auto idx = util::convertBEU16(*(id_range_offsets.data() + i + range_ofs / 2 + (codepoint - start)));
					return (delta + idx) & 0xffff;
				}
				else
				{
					return (delta + codepoint) & 0xffff;
				}
			}
		}

		return 0;
	}

	static uint32_t find_in_subtable_6(zst::byte_span subtable, uint32_t codepoint)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 6);

		auto len = consume_u16(subtable);
		auto lang = consume_u16(subtable);

		(void) len;
		(void) lang;

		auto first = consume_u16(subtable);
		auto count = consume_u16(subtable);

		if(codepoint < first || codepoint >= first + count)
			return 0;

		return subtable.cast<uint16_t>()[codepoint - first];
	}

	static uint32_t find_in_subtable_10(zst::byte_span subtable, uint32_t codepoint)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 10);

		consume_u16(subtable);  // reserved
		auto len = consume_u32(subtable);
		auto lang = consume_u32(subtable);

		(void) len;
		(void) lang;

		auto first = consume_u32(subtable);
		auto count = consume_u32(subtable);

		if(codepoint < first || codepoint >= first + count)
			return 0;

		// alignment may be a problem.
		uint32_t ret = 0;

		auto array = subtable.cast<uint32_t>();
		memcpy(&ret, array.data() + (codepoint - first), sizeof(uint32_t));
		return ret;
	}


	static uint32_t find_in_subtable_12_or_13(zst::byte_span subtable, uint32_t codepoint)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 12 || fmt == 13);

		consume_u16(subtable);  // reserved
		auto len = consume_u32(subtable);
		auto lang = consume_u32(subtable);

		(void) len;
		(void) lang;

		auto num_groups = consume_u32(subtable);

		struct Group
		{
			uint32_t first;
			uint32_t last;
			uint32_t glyphid;
		} __attribute__((packed));

		static_assert(sizeof(Group) == 3 * sizeof(uint32_t), "your compiler is broken");

		// the group_array starts right after the number of groups
		auto group_array = subtable.cast<Group>();

		size_t low = 0;
		size_t high = num_groups;

		while(low < high)
		{
			size_t grp_num = (low + high) / 2;
			auto grp = group_array[grp_num];

			auto fst = util::convertBEU32(grp.first);
			auto lst = util::convertBEU32(grp.last);
			auto gid = util::convertBEU32(grp.glyphid);

			if(fst <= codepoint && codepoint <= lst)
			{
				if(fmt == 12) return gid + (codepoint - fst);
				else          return gid;
			}
			else if(codepoint < fst)
			{
				high = grp_num;
			}
			else
			{
				low = grp_num + 1;
			}
		}

		return 0;
	}
























	// TODO: we can probably also optimise this API so it returns more than a single
	// codepoint at a time...

	// note that this does not do any caching -- that should be done at the PDF level.
	uint32_t FontFile::getGlyphIndexForCodepoint(uint32_t codepoint) const
	{
		auto subtable = zst::byte_span(this->file_bytes, this->file_size)
			.drop(this->preferred_cmap.file_offset);

		switch(this->preferred_cmap.format)
		{
			case 0:  return find_in_subtable_0(subtable, codepoint);
			case 4:  return find_in_subtable_4(subtable, codepoint);
			case 6:  return find_in_subtable_6(subtable, codepoint);
			case 10: return find_in_subtable_10(subtable, codepoint);
			case 12: return find_in_subtable_12_or_13(subtable, codepoint);
			case 13: return find_in_subtable_12_or_13(subtable, codepoint);
			default:
				return 0;
		}
	}
}
