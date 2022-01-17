// font.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <set>
#include <map>
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

		uint32_t getGlyphIdFromCodepoint(uint32_t codepoint) const;
		std::optional<font::KerningPair> getKerningForGlyphs(uint32_t glyph1, uint32_t glyph2) const;

		std::optional<font::GlyphLigatureSet> getLigaturesForGlyph(uint32_t glyph) const;

		// this is necessary because ligature substitutions can result in obtaining glyphs that
		// didn't come from `getGlyphIdFromCodepoint`, so we need to manually read its width
		// and put it into the pdf, if not it uses the default width.
		void loadMetricsForGlyph(uint32_t glyph) const;

		font::GlyphMetrics getMetricsForGlyph(uint32_t glyph) const;

		font::FontMetrics getFontMetrics() const;

		// this has a similar function to `loadMetricsForGlyph`; we need to keep track of which
		// ligatures are used, so we know which ones we should output to the pdf.
		void markLigatureUsed(const font::GlyphLigature& ligature) const;

		// get the name that we should put in the Resource dictionary of a page that uses this font.
		std::string getFontResourceName() const;

		// abstracts away the scaling by units_per_em, to go from font units to pdf units
		// this converts the metric to a **concrete size** (in pdf units, aka 1/72 inches)
		Scalar scaleFontMetricForFontSize(double metric, Scalar font_size) const;

		// this converts the metric to an **abstract size**, which is the text space. when
		// drawing text, the /Tf directive already specifies the font scale!
		Scalar scaleFontMetricForPDFTextSpace(double metric) const;


		int font_type = 0;
		int encoding_kind = 0;

		static Font* fromBuiltin(Document* doc, zst::str_view name);
		static Font* fromFontFile(Document* doc, font::FontFile* font);

		static constexpr int FONT_TYPE1         = 1;
		static constexpr int FONT_TRUETYPE      = 2;
		static constexpr int FONT_CFF_CID       = 3;
		static constexpr int FONT_TRUETYPE_CID  = 4;

		static constexpr int ENCODING_WIN_ANSI  = 1;
		static constexpr int ENCODING_CID       = 2;

	private:
		Font();
		Dictionary* font_dictionary = 0;

		void writeUnicodeCMap(Document* doc) const;

		mutable std::map<uint32_t, uint32_t> cmap_cache { };
		mutable std::map<uint32_t, font::GlyphMetrics> glyph_metrics { };

		// TODO: this results in a bunch of copying. since we have an owned copy
		// of every GlyphLigatureSet that is used, we should be able to refer to our internal
		// copy, via a pointer or something.
		mutable std::vector<font::GlyphLigature> used_ligatures { };
		mutable std::map<uint32_t, uint32_t> reverse_cmap { };

		std::map<uint32_t, font::GlyphLigatureSet> glyph_ligatures { };
		std::map<std::pair<uint32_t, uint32_t>, font::KerningPair> kerning_pairs { };

		// the name that goes into the Resource << >> dict in a page. This is a unique name
		// that we get from the Document when the font is created.
		std::string font_resource_name { };

		// only used for embedded fonts
		font::FontFile* source_file = 0;
		Array* glyph_widths_array = 0;

		Stream* embedded_contents = 0;
		Stream* unicode_cmap = 0;

		// what goes in BaseName. for subsets, this includes the ABCDEF+ part.
		std::string pdf_font_name;

		// pool needs to be a friend because it needs the constructor
		template <typename> friend struct util::Pool;
	};
}
