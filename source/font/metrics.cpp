// metrics.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"  // for convertBEU16
#include "types.h" // for GlyphId

#include "font/truetype.h"    // for BoundingBox, getGlyphBoundingBox
#include "font/font_file.h"   // for GlyphMetrics, FontFile, FontMetrics
#include "font/font_scalar.h" // for FontScalar, font_design_space

namespace font
{
	std::vector<GlyphInfo> FontFile::getGlyphInfosForString(zst::wstr_view text) const
	{
		// first, convert all codepoints to glyphs
		std::vector<GlyphId> glyphs {};
		for(char32_t cp : text)
			glyphs.push_back(this->getGlyphIndexForCodepoint(cp));

		using font::Tag;
		font::off::FeatureSet features {};

		// REMOVE: this is just for testing!
		features.script = Tag("cyrl");
		features.language = Tag("BGR ");
		features.enabled_features = { Tag("kern"), Tag("liga"), Tag("locl") };

		auto span = [](auto& foo) {
			return zst::span<GlyphId>(foo.data(), foo.size());
		};

		// next, use GSUB to perform substitutions.
		// ignore glyph mappings because we only care about the final result
		glyphs = font::off::performSubstitutionsForGlyphSequence(this, span(glyphs), features).glyphs;

		// next, get base metrics for each glyph.
		std::vector<GlyphInfo> glyph_infos {};
		for(auto g : glyphs)
		{
			GlyphInfo info {};
			info.gid = g;
			info.metrics = this->getGlyphMetrics(g);
			glyph_infos.push_back(std::move(info));
		}

		// finally, use GPOS
		auto adjustment_map = font::off::getPositioningAdjustmentsForGlyphSequence(this, span(glyphs), features);
		for(auto& [i, adj] : adjustment_map)
		{
			auto& info = glyph_infos[i];
			info.adjustments.horz_advance += adj.horz_advance;
			info.adjustments.vert_advance += adj.vert_advance;
			info.adjustments.horz_placement += adj.horz_placement;
			info.adjustments.vert_placement += adj.vert_placement;
		}

		return glyph_infos;
	}

	GlyphMetrics FontFile::getGlyphMetrics(GlyphId glyph_id) const
	{
		auto u16_array = m_hmtx_table.cast<uint16_t>();

		GlyphMetrics ret {};
		auto gid32 = static_cast<uint32_t>(glyph_id);

		// note the *2, because there's also an 2-byte "lsb".
		if(gid32 >= m_num_hmetrics)
		{
			// the remaining glyphs not in the array use the last value.
			ret.horz_advance = FontScalar(util::convertBEU16(u16_array[(m_num_hmetrics - 1) * 2]));

			// there is an array of lsbs for glyph_ids > num_hmetrics
			auto lsb_array = m_hmtx_table.drop(2 * m_num_hmetrics);
			auto tmp = lsb_array[gid32 - m_num_hmetrics];

			ret.left_side_bearing = FontScalar((int16_t) util::convertBEU16(tmp));
		}
		else
		{
			ret.horz_advance = FontScalar(util::convertBEU16(u16_array[gid32 * 2]));
			ret.left_side_bearing = FontScalar((int16_t) util::convertBEU16(u16_array[gid32 * 2 + 1]));
		}

		// now, figure out xmin and xmax
		if(this->hasTrueTypeOutlines())
		{
			auto bb = truetype::getGlyphBoundingBox(m_truetype_data.get(), glyph_id);
			ret.xmin = FontScalar(bb.xmin);
			ret.ymin = FontScalar(bb.ymin);
			ret.xmax = FontScalar(bb.xmax);
			ret.ymax = FontScalar(bb.ymax);

			// calculate RSB
			ret.right_side_bearing = ret.horz_advance - ret.left_side_bearing - (ret.xmax - ret.xmin);
		}
		else if(this->hasCffOutlines())
		{
			// sap::warn("font/metrics", "bounding-box metrics for CFF-outline fonts are not supported yet!");

			// well, we're shit out of luck.
			// best effort, i guess?
			ret.xmin = FontScalar(m_metrics.xmin);
			ret.xmax = FontScalar(m_metrics.xmax);
			ret.ymin = FontScalar(m_metrics.ymin);
			ret.ymax = FontScalar(m_metrics.ymax);
		}
		else
		{
			sap::internal_error("unsupported outline type?!");
		}

		return ret;
	}
}
