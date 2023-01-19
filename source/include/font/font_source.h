// font_source.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"

#include "font/metrics.h"
#include "font/features.h"

namespace font
{
	struct CharacterMapping
	{
		util::hashmap<char32_t, GlyphId> forward;
		util::hashmap<GlyphId, char32_t> reverse;
	};

	struct FontSource
	{
		virtual ~FontSource() { }

		virtual std::string name() const = 0;

		size_t numGlyphs() const { return m_num_glyphs; }
		const FontMetrics& metrics() const { return m_metrics; }
		const CharacterMapping& characterMapping() const { return m_character_mapping; }

		GlyphId getGlyphIndexForCodepoint(char32_t codepoint) const;
		GlyphMetrics getGlyphMetrics(GlyphId glyphId) const;

		// returns the GlyphInfos for the given glyph string. note that this *DOES NOT* perform GSUB/morx, ie.
		// ligatures and/or language-specific glyphs are not done here -- they are handled by the PDF layer.
		// this only looks up positioning stuff, eg. GPOS and kern.
		std::vector<GlyphInfo> getGlyphInfosForSubstitutedString(zst::span<GlyphId> glyphs, const FeatureSet& features) const;


		bool isGlyphUsed(GlyphId glyph_id) const;
		void markGlyphAsUsed(GlyphId glyph_id) const;
		const util::hashset<GlyphId>& usedGlyphs() const;

		virtual bool isBuiltin() const = 0;
		virtual std::map<size_t, GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(zst::span<GlyphId> glyphs,
		    const font::FeatureSet& features) const = 0;
		virtual std::optional<SubstitutedGlyphString> performSubstitutionsForGlyphSequence(zst::span<GlyphId> glyphs,
		    const font::FeatureSet& features) const = 0;

	protected:
		virtual GlyphMetrics get_glyph_metrics_impl(GlyphId glyphid) const = 0;

		size_t m_num_glyphs = 0;
		FontMetrics m_metrics {};

		CharacterMapping m_character_mapping {};

		mutable util::hashset<GlyphId> m_used_glyphs {};
		mutable util::hashmap<GlyphId, GlyphMetrics> m_glyph_metrics {};
	};
}
