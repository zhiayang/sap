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

		size_t numGlyphs() const { return m_num_glyphs; }
		const FontMetrics& metrics() const { return m_metrics; }
		const CharacterMapping& characterMapping() const { return m_character_mapping; }

		GlyphId getGlyphIndexForCodepoint(char32_t codepoint) const;
		GlyphMetrics getGlyphMetrics(GlyphId glyphId) const;
		FontVector2d getWordSize(zst::wstr_view word) const;
		std::vector<GlyphInfo> getGlyphInfosForString(zst::wstr_view text) const;

		bool isGlyphUsed(GlyphId glyph_id) const { return m_used_glyphs.contains(glyph_id); }
		void markGlyphAsUsed(GlyphId glyph_id) const { m_used_glyphs.insert(glyph_id); }
		const auto& usedGlyphs() const { return m_used_glyphs; }


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
		mutable util::hashmap<std::u32string, FontVector2d> m_word_size_cache {};
		mutable util::hashmap<std::u32string, std::vector<GlyphInfo>> m_glyph_infos_cache {};
	};
}
