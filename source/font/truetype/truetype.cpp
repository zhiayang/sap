// truetype.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"  // for convertBEU16
#include "types.h" // for GlyphId

#include "font/misc.h"
#include "font/truetype.h"  // for Glyph, TTData, BoundingBox, getGlyphBound...
#include "font/font_file.h" // for consume_u16, consume_u8, FontFile, consum...

namespace font::truetype
{
	static void parse_glyph_components(TTData* tt, Glyph& glyph)
	{
		auto data = glyph.glyph_data;
		if(data.size() == 0)
			return;

		// num_contours >= 0 -> simple glyph, no components
		int16_t num_contours = consume_i16(data);
		if(num_contours >= 0)
			return;

		// remove the xmin/max stuff
		data.remove_prefix(2 * 4);

		while(data.size() > 0)
		{
			auto flags = consume_u16(data);
			auto gid = consume_u16(data);

			// ARG_1_AND_2_ARE_WORDS
			if(flags & 0x0001)
				consume_u16(data), consume_u16(data);
			else
				consume_u8(data), consume_u8(data);

			// WE_HAVE_A_SCALE (0x8); WE_HAVE_AN_X_AND_Y_SCALE (0x40); WE_HAVE_A_TWO_BY_TWO (0x80)
			if(flags & 0x0008)
				consume_u16(data);
			else if(flags & 0x0040)
				consume_u16(data), consume_u16(data);
			else if(flags & 0x0080)
				consume_u16(data), consume_u16(data), consume_u16(data), consume_u16(data);

			// TODO: handle `USE_MY_METRICS` flag, or see if it's even useful. isn't there hmtx for that?

			glyph.component_gids.insert(gid);

			// MORE_COMPONENTS
			if(!(flags & 0x20))
				break;
		}
	}

	BoundingBox getGlyphBoundingBox(TTData* tt, GlyphId glyph_id)
	{
		if(static_cast<uint32_t>(glyph_id) >= tt->glyphs.size())
			sap::error("font/ttf", "glyph index '{}' out of range (max is '{}')", glyph_id, tt->glyphs.size() - 1);

		auto glyph_data = tt->glyphs[static_cast<uint32_t>(glyph_id)].glyph_data;

		// layout: numContours (16); xmin (16); ymin (16); xmax (16); ymax (16);
		auto foozle = glyph_data.cast<uint16_t>();

		BoundingBox ret {};

		ret.xmin = (int16_t) util::convertBEU16(foozle[1]);
		ret.ymin = (int16_t) util::convertBEU16(foozle[2]);
		ret.xmax = (int16_t) util::convertBEU16(foozle[3]);
		ret.ymax = (int16_t) util::convertBEU16(foozle[4]);

		return ret;
	}
}

namespace font
{
	void FontFile::parse_glyf_table(const Table& glyf_table)
	{
		if(m_outline_type != OUTLINES_TRUETYPE)
			sap::internal_error("found 'glyf' table in file with non-truetype outlines");

		assert(m_truetype_data != nullptr);
		auto data = this->bytes().drop(glyf_table.offset).take(glyf_table.length);

		m_truetype_data->glyf_data = data;
	}

	void FontFile::parse_loca_table(const Table& tbl)
	{
		if(m_outline_type != OUTLINES_TRUETYPE)
			sap::internal_error("found 'loca' table in file with non-truetype outlines");

		assert(m_truetype_data != nullptr);
		auto loca_table = this->bytes().drop(tbl.offset).take(tbl.length);

		// the loca table should come after the glyph table!
		assert(m_truetype_data->glyf_data.size() > 0);

		bool half = m_truetype_data->loca_bytes_per_entry == 2;
		auto get_ofs = [&]() -> uint32_t {
			if(half)
				return 2 * consume_u16(loca_table);
			else
				return 1 * consume_u32(loca_table);
		};

		auto peek_ofs = [&]() -> uint32_t {
			if(half)
				return 2 * peek_u16(loca_table);
			else
				return 1 * peek_u32(loca_table);
		};

		for(uint16_t i = 0; i < m_num_glyphs; i++)
		{
			truetype::Glyph glyph {};
			glyph.gid = i;

			auto offset = get_ofs();
			auto next = peek_ofs();

			glyph.glyph_data = m_truetype_data->glyf_data.drop(offset).take(next - offset);

			m_truetype_data->glyphs.push_back(std::move(glyph));
			truetype::parse_glyph_components(m_truetype_data.get(), m_truetype_data->glyphs[i]);
		}
	}
}
