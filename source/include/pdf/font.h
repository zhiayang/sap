// font.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <set>
#include <map>
#include <unordered_set>

#include <zst.h>

#include "pool.h"

#include "pdf/units.h"
#include "font/font.h"

namespace pdf
{
	struct Array;
	struct Stream;
	struct Document;
	struct Dictionary;

	// PDF 1.7: 9.2.4 Glyph Positioning and Metrics
	// ... the units of glyph space are one-thousandth of a unit of text space ...
	static constexpr double GLYPH_SPACE_UNITS = 1000.0;

	struct Font
	{
		Dictionary* serialise(Document* doc) const;

		GlyphId getGlyphIdFromCodepoint(Codepoint codepoint) const;

		void markGlyphAsUsed(GlyphId glyph) const;

		font::GlyphMetrics getMetricsForGlyph(GlyphId glyph) const;

		// add an explicit mapping from glyph id to a list of codepoints. This is useful for
		// ligature substitutions (eg. 'ffi' -> 'f', 'f', 'i') and for single replacements.
		void addGlyphUnicodeMapping(GlyphId glyph, std::vector<Codepoint> codepoints) const;

		font::FontMetrics getFontMetrics() const;

		// get the name that we should put in the Resource dictionary of a page that uses this font.
		std::string getFontResourceName() const;

		// abstracts away the scaling by units_per_em, to go from font units to pdf units
		// this converts the metric to a **concrete size** (in pdf units, aka 1/72 inches)
		Scalar scaleMetricForFontSize(double metric, Scalar font_size) const;

		// this converts the metric to an **abstract size**, which is the text space. when
		// drawing text, the /Tf directive already specifies the font scale!
		Scalar scaleMetricForPDFTextSpace(double metric) const;

		/*
		    A very thin wrapper around the identically-named methods taking a FontFile
		*/
		std::map<size_t, font::GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(zst::span<GlyphId> glyphs,
			const font::off::FeatureSet& features) const;

		std::vector<GlyphId> performSubstitutionsForGlyphSequence(zst::span<GlyphId> glyphs,
			const font::off::FeatureSet& features) const;


		int font_type = 0;
		int encoding_kind = 0;

		static Font* fromBuiltin(Document* doc, zst::str_view name);
		static Font* fromFontFile(Document* doc, font::FontFile* font);

		static constexpr int FONT_TYPE1 = 1;
		static constexpr int FONT_TRUETYPE = 2;
		static constexpr int FONT_CFF_CID = 3;
		static constexpr int FONT_TRUETYPE_CID = 4;

		static constexpr int ENCODING_WIN_ANSI = 1;
		static constexpr int ENCODING_CID = 2;

	private:
		Font();
		Dictionary* font_dictionary = 0;

		void writeUnicodeCMap(Document* doc) const;
		void writeCIDSet(Document* doc) const;

		mutable std::unordered_set<GlyphId> m_used_glyphs {};
		mutable std::map<GlyphId, font::GlyphMetrics> m_glyph_metrics {};
		mutable std::map<GlyphId, std::vector<Codepoint>> m_extra_unicode_mappings {};

		// the name that goes into the Resource << >> dict in a page. This is a unique name
		// that we get from the Document when the font is created.
		std::string font_resource_name {};

		// only used for embedded fonts
		font::FontFile* source_file = 0;
		Array* glyph_widths_array = 0;

		Stream* embedded_contents = 0;
		Stream* unicode_cmap = 0;
		Stream* cidset = 0;

		// what goes in BaseName. for subsets, this includes the ABCDEF+ part.
		std::string pdf_font_name;

		// pool needs to be a friend because it needs the constructor
		template <typename>
		friend struct util::Pool;
	};
}
