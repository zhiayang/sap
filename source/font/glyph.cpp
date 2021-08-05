// glyph.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "util.h"
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
		}
		else
		{
			ret.horz_advance = util::convertBEU16(u16_array[glyphId * 2]);
		}

		// again, pdf wants this in typographic units but these are in font design units,
		// so do the units_per_em scale.

		ret.horz_advance = (ret.horz_advance * 1000) / this->metrics.units_per_em;
		ret.vert_advance = (ret.vert_advance * 1000) / this->metrics.units_per_em;

		return ret;
	}
}
