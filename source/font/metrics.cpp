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

	FontVector2d FontSource::getWordSize(zst::wstr_view text) const
	{
		if(auto it = m_word_size_cache.find(text); it != m_word_size_cache.end())
			return it->second;

		FontVector2d size;
		size.y() = m_metrics.default_line_spacing;

		const auto& glyphs = this->getGlyphInfosForString(text);
		for(auto& g : glyphs)
			size.x() += g.metrics.horz_advance + g.adjustments.horz_advance;

		m_word_size_cache.emplace(text.str(), size);
		return size;
	}

	std::vector<GlyphInfo> FontSource::getGlyphInfosForString(zst::wstr_view text) const
	{
		if(auto it = m_glyph_infos_cache.find(text); it != m_glyph_infos_cache.end())
			return it->second;

		// first, convert all codepoints to glyphs
		std::vector<GlyphId> glyphs {};
		for(char32_t cp : text)
			glyphs.push_back(this->getGlyphIndexForCodepoint(cp));

		using font::Tag;
		font::FeatureSet features {};

		// REMOVE: this is just for testing!
		features.script = Tag("cyrl");
		features.language = Tag("BGR ");
		features.enabled_features = { Tag("kern"), Tag("liga"), Tag("locl") };

		auto span = [](auto& foo) {
			return zst::span<GlyphId>(foo.data(), foo.size());
		};

		// next, use GSUB to perform substitutions.
		// ignore glyph mappings because we only care about the final result
		if(auto subst = this->performSubstitutionsForGlyphSequence(span(glyphs), features); subst.has_value())
			glyphs = std::move(subst->glyphs);

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

		auto res = m_glyph_infos_cache.emplace(text.str(), std::move(glyph_infos));
		return res.first->second;
	}
}
