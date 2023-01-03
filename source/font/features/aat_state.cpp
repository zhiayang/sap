// aat_state.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "font/aat.h"
#include "font/misc.h"

namespace font::aat
{
	static void parse_class_subtable_stx(StateTable& state_table, zst::byte_span buf, size_t num_glyphs)
	{
		auto lookup = *parseLookupTable(buf, num_glyphs);
		for(auto& [a, b] : lookup.map)
			state_table.glyph_classes[a] = static_cast<uint16_t>(b);
	}

	static void parse_class_subtable(StateTable& state_table, zst::byte_span buf)
	{
		auto first_glyph = consume_u16(buf);
		auto num_glyphs = consume_u16(buf);

		for(auto i = 0u; i < num_glyphs; i++)
			state_table.glyph_classes[GlyphId(first_glyph + i)] = consume_u8(buf);
	}

	static StateTable parse_state_table(zst::byte_span& buf, bool is_stx, size_t num_font_glyphs)
	{
		auto table_start = buf;

		StateTable ret;

		zst::byte_span class_table;
		zst::byte_span state_table;
		zst::byte_span entry_table;

		if(is_stx)
		{
			ret.num_classes = consume_u32(buf);
			class_table = table_start.drop(consume_u32(buf));
			state_table = table_start.drop(consume_u32(buf));
			entry_table = table_start.drop(consume_u32(buf));
		}
		else
		{
			ret.num_classes = consume_u16(buf);
			class_table = table_start.drop(consume_u16(buf));
			state_table = table_start.drop(consume_u16(buf));
			entry_table = table_start.drop(consume_u16(buf));
		}

		if(is_stx)
			parse_class_subtable_stx(ret, class_table, num_font_glyphs);
		else
			parse_class_subtable(ret, class_table);

		ret.entry_array = entry_table;
		ret.state_array = state_table;
		ret.state_row_size = (is_stx ? sizeof(uint16_t) : sizeof(uint8_t)) * ret.num_classes;
		ret.is_extended = is_stx;

		return ret;
	}


	StateTable parseStateTable(zst::byte_span& buf, size_t num_font_glyphs)
	{
		return parse_state_table(buf, /* is_stx: */ false, num_font_glyphs);
	}

	StateTable parseExtendedStateTable(zst::byte_span& buf, size_t num_font_glyphs)
	{
		return parse_state_table(buf, /* is_stx: */ true, num_font_glyphs);
	}
}
