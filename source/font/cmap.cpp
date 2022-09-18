// cmap.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"
#include "error.h"

#include "font/font.h"

namespace font
{
	static CharacterMapping read_subtable_0(zst::byte_span subtable)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 0);

		auto len = consume_u16(subtable);
		assert(len == 3 * sizeof(uint16_t) + 256 * sizeof(uint8_t));

		consume_u16(subtable);  // lang (ignored)

		CharacterMapping mapping {};
		for(uint32_t cp = 0; cp < 256; cp++)
		{
			auto gid = GlyphId { subtable[cp] };
			mapping.forward[Codepoint { cp }] = gid;
			mapping.reverse[gid] = Codepoint { cp };
		}

		return mapping;
	}

	static CharacterMapping read_subtable_4(zst::byte_span subtable)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 4);

		consume_u16(subtable);  // len (ignored)
		consume_u16(subtable);  // lang (ignored)

		// what the fuck is this format? fmt 12 is so much better.
		auto seg_count = consume_u16(subtable) / 2u;
		consume_u16(subtable);  // search_range (ignored)
		consume_u16(subtable);  // entry_selector (ignored)
		consume_u16(subtable);  // range_shift (ignored)

		auto u16_array = subtable.cast<uint16_t>();
		auto end_codes = u16_array.take_prefix(seg_count);
		u16_array.remove_prefix(1);  // reserved

		auto start_codes = u16_array.take_prefix(seg_count);
		auto id_deltas = u16_array.take_prefix(seg_count);
		auto id_range_offsets = u16_array.take_prefix(seg_count);

		CharacterMapping mapping {};
		for(size_t i = 0; i < seg_count; i++)
		{
			auto start = util::convertBEU16(start_codes[i]);
			auto end = util::convertBEU16(end_codes[i]);
			auto delta = util::convertBEU16(id_deltas[i]);
			auto range_ofs = util::convertBEU16(id_range_offsets[i]);

			for(uint32_t cp = start; cp <= end; cp++)
			{
				if(range_ofs != 0)
				{
					auto idx = util::convertBEU16(*(id_range_offsets.data() + i + range_ofs / 2 + (cp - start)));
					auto gid = GlyphId { static_cast<uint32_t>((delta + idx) & 0xffff) };
					mapping.forward[Codepoint { cp }] = gid;
					mapping.reverse[gid] = Codepoint { cp };
				}
				else
				{
					auto gid = GlyphId { static_cast<uint32_t>((delta + cp) & 0xffff) };
					mapping.forward[Codepoint { cp }] = gid;
					mapping.reverse[gid] = Codepoint { cp };
				}
			}
		}

		return mapping;
	}

	static CharacterMapping read_subtable_6(zst::byte_span subtable)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 6);

		consume_u16(subtable);  // len
		consume_u16(subtable);  // lang

		auto first = consume_u16(subtable);
		auto count = consume_u16(subtable);

		CharacterMapping mapping {};
		for(size_t i = 0; i < count; i++)
		{
			auto cp = Codepoint { static_cast<uint32_t>(first + i) };
			auto gid = GlyphId { subtable.cast<uint16_t>()[i] };

			mapping.forward[cp] = gid;
			mapping.reverse[gid] = cp;
		}
		return mapping;
	}

	static CharacterMapping read_subtable_10(zst::byte_span subtable)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 10);

		consume_u16(subtable);  // reserved
		consume_u32(subtable);  // len
		consume_u32(subtable);  // lang

		auto first = consume_u32(subtable);
		auto count = consume_u32(subtable);

		CharacterMapping mapping {};
		for(size_t i = 0; i < count; i++)
		{
			auto cp = Codepoint { static_cast<uint32_t>(first + i) };
			auto gid = GlyphId { subtable.cast<uint32_t>()[i] };

			mapping.forward[cp] = gid;
			mapping.reverse[gid] = cp;
		}

		return mapping;
	}

	static CharacterMapping read_subtable_12_or_13(zst::byte_span subtable)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 12 || fmt == 13);

		consume_u16(subtable);  // reserved
		consume_u32(subtable);  // len
		consume_u32(subtable);  // lang

		auto num_groups = consume_u32(subtable);

		CharacterMapping mapping {};
		for(size_t i = 0; i < num_groups; i++)
		{
			auto first = consume_u32(subtable);
			auto last = consume_u32(subtable);
			auto g = consume_u32(subtable);

			for(auto x = first; x <= last; x++)
			{
				auto cp = Codepoint { x };

				GlyphId gid {};
				if(fmt == 12)
					gid = GlyphId { g + (x - first) };
				else
					gid = GlyphId { g };

				mapping.forward[cp] = gid;
				mapping.reverse[gid] = cp;
			}
		}

		return mapping;
	}






	CharacterMapping readCMapTable(zst::byte_span table)
	{
		auto format = peek_u16(table);
		switch(format)
		{
			case 0:
				return read_subtable_0(table);
			case 4:
				return read_subtable_4(table);
			case 6:
				return read_subtable_6(table);
			case 10:
				return read_subtable_10(table);
			case 12:
				return read_subtable_12_or_13(table);
			case 13:
				return read_subtable_12_or_13(table);
			default:
				sap::warn("font/off", "no supported cmap table! no glyphs will be available");
				return {};
		}
	}

	GlyphId FontFile::getGlyphIndexForCodepoint(Codepoint codepoint) const
	{
		auto& fwd = this->character_mapping.forward;
		if(auto it = fwd.find(codepoint); it != fwd.end())
			return it->second;
		else
			return GlyphId::notdef;
	}
}
