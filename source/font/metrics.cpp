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
	GlyphMetrics FontSource::getGlyphMetrics(GlyphId glyph_id) const
	{
		if(auto it = m_glyph_metrics.find(glyph_id); it != m_glyph_metrics.end())
			return it->second;

		auto metrics = this->get_glyph_metrics_impl(glyph_id);
		return m_glyph_metrics.emplace(glyph_id, std::move(metrics)).first->second;
	}

	std::vector<GlyphInfo> FontSource::getGlyphInfosForSubstitutedString(zst::span<GlyphId> glyphs,
	    const FeatureSet& features) const
	{
		auto span = [](auto& foo) {
			return zst::span<GlyphId>(foo.data(), foo.size());
		};

		// next, get base metrics for each glyph.
		std::vector<GlyphInfo> glyph_infos {};
		for(auto g : glyphs)
		{
			glyph_infos.push_back(GlyphInfo {
			    .gid = g,
			    .metrics = this->getGlyphMetrics(g),
			});
		}

		// finally, use GPOS
		auto adjustment_map = this->getPositioningAdjustmentsForGlyphSequence(span(glyphs), features);
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
}
