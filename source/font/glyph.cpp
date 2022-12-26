// glyph.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"  // for convertBEU16
#include "error.h" // for internal_error
#include "types.h" // for GlyphId

#include "font/font.h"     // for GlyphMetrics, FontFile, FontMetrics, Font...
#include "font/truetype.h" // for BoundingBox, getGlyphBoundingBox

namespace font
{
	GlyphMetrics FontFile::getGlyphMetrics(GlyphId glyph_id) const
	{
		auto u16_array = this->hmtx_table.cast<uint16_t>();

		GlyphMetrics ret {};
		auto gid32 = static_cast<uint32_t>(glyph_id);

		// note the *2, because there's also an 2-byte "lsb".
		if(gid32 >= this->num_hmetrics)
		{
			// the remaining glyphs not in the array use the last value.
			ret.horz_advance = FontScalar(util::convertBEU16(u16_array[(this->num_hmetrics - 1) * 2]));

			// there is an array of lsbs for glyph_ids > num_hmetrics
			auto lsb_array = this->hmtx_table.drop(2 * this->num_hmetrics);
			auto tmp = lsb_array[gid32 - this->num_hmetrics];

			ret.left_side_bearing = FontScalar((int16_t) util::convertBEU16(tmp));
		}
		else
		{
			ret.horz_advance = FontScalar(util::convertBEU16(u16_array[gid32 * 2]));
			ret.left_side_bearing = FontScalar((int16_t) util::convertBEU16(u16_array[gid32 * 2 + 1]));
		}

		// now, figure out xmin and xmax
		if(this->outline_type == OUTLINES_TRUETYPE)
		{
			auto bb = truetype::getGlyphBoundingBox(this->truetype_data, glyph_id);
			ret.xmin = FontScalar(bb.xmin);
			ret.ymin = FontScalar(bb.ymin);
			ret.xmax = FontScalar(bb.xmax);
			ret.ymax = FontScalar(bb.ymax);

			// calculate RSB
			ret.right_side_bearing = ret.horz_advance - ret.left_side_bearing - (ret.xmax - ret.xmin);
		}
		else if(this->outline_type == OUTLINES_CFF)
		{
			// sap::warn("font/metrics", "bounding-box metrics for CFF-outline fonts are not supported yet!");

			// well, we're shit out of luck.
			// best effort, i guess?
			auto metrics = this->metrics;

			ret.xmin = FontScalar(metrics.xmin);
			ret.xmax = FontScalar(metrics.xmax);
			ret.ymin = FontScalar(metrics.ymin);
			ret.ymax = FontScalar(metrics.ymax);
		}
		else
		{
			sap::internal_error("unsupported outline type?!");
		}

		return ret;
	}
}
