// builtin_font.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "font/font_source.h"

template <>
struct std::hash<std::pair<GlyphId, GlyphId>>
{
	size_t operator()(std::pair<GlyphId, GlyphId> p) const
	{
		return std::hash<uint64_t>()((static_cast<uint64_t>(p.first) << 32) | (static_cast<uint64_t>(p.second)));
	}
};

namespace pdf
{
	struct BuiltinFont : font::FontSource
	{
		enum Core14
		{
			TimesRoman,
			TimesBold,
			TimesItalic,
			TimesBoldItalic,

			Courier,
			CourierBold,
			CourierOblique,
			CourierBoldOblique,

			Helvetica,
			HelveticaBold,
			HelveticaOblique,
			HelveticaBoldOblique,

			Symbol,
			ZapfDingbats,
		};

		virtual std::string name() const override { return m_name; }
		Core14 kind() const { return m_kind; }

		GlyphId getFirstGlyphId() const { return m_first_glyph_id; }
		GlyphId getLastGlyphId() const { return m_last_glyph_id; }

		virtual bool isBuiltin() const override { return true; }

		virtual util::hashmap<size_t, font::GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(
		    zst::span<GlyphId> glyphs,
		    const font::FeatureSet& features) const override;
		virtual std::optional<font::SubstitutedGlyphString> performSubstitutionsForGlyphSequence(zst::span<GlyphId>
		                                                                                             glyphs,
		    const font::FeatureSet& features) const override;

		virtual font::GlyphMetrics get_glyph_metrics_impl(GlyphId glyphid) const override;

		static std::unique_ptr<BuiltinFont> get(Core14 font);

	private:
		BuiltinFont(Core14 which);

		Core14 m_kind;

		std::string m_name;
		GlyphId m_first_glyph_id;
		GlyphId m_last_glyph_id;

		util::hashmap<std::pair<GlyphId, GlyphId>, font::GlyphAdjustment> m_kerning_pairs;

		// note: we know that the ligatures are only in pairs... and they're only ever 'fi' and 'fl'
		util::hashmap<std::pair<GlyphId, GlyphId>, GlyphId> m_ligature_pairs;
	};
}
