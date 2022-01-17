// glyph.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"
#include "error.h"
#include "font/font.h"

namespace font
{
	GlyphMetrics FontFile::getGlyphMetrics(uint32_t glyphId) const
	{
		auto buf = zst::byte_span(this->file_bytes, this->file_size);
		buf.remove_prefix(this->hmtx_table.offset);

		auto u16_array = buf.cast<uint16_t>();

		GlyphMetrics ret { };

		// note the *2, because there's also an 2-byte "lsb".
		if(glyphId >= this->num_hmetrics)
		{
			// the remaining glyphs not in the array use the last value.
			ret.horz_advance = util::convertBEU16(u16_array[(this->num_hmetrics - 1) * 2]);

			// there is an array of lsbs for glyph_ids > num_hmetrics
			auto lsb_array = buf.drop(2 * this->num_hmetrics);
			auto tmp = lsb_array[glyphId - this->num_hmetrics];

			ret.left_side_bearing = (int16_t) util::convertBEU16(tmp);
		}
		else
		{
			ret.horz_advance = util::convertBEU16(u16_array[glyphId * 2]);
			ret.left_side_bearing = (int16_t) util::convertBEU16(u16_array[glyphId * 2 + 1]);
		}

		// now, figure out xmin and xmax
		if(this->outline_type == OUTLINES_TRUETYPE)
		{
		}
		else if(this->outline_type == OUTLINES_CFF)
		{
			// well, we're shit out of luck.
			// best effort, i guess?

		}
		else
		{
			sap::internal_error("unsupported outline type?!");
		}

		return ret;
	}
}
