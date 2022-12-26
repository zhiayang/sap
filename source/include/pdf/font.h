// font.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "types.h" // for GlyphId

#include "pdf/units.h" // for TextSpace1d, Scalar, GlyphSpace1d, pdf_ty...

#include "font/font.h"     // for GlyphMetrics, FontMetrics, Stream, FontFile
#include "font/scalar.h"   // for font_design_space, FontScalar
#include "font/features.h" // for FeatureSet, GlyphAdjustment

namespace pdf
{
	struct Array;
	struct Stream;
	struct Document;
	struct Dictionary;

	struct Font
	{
		Dictionary* serialise(Document* doc) const;

		GlyphId getGlyphIdFromCodepoint(char32_t codepoint) const;

		void markGlyphAsUsed(GlyphId glyph) const;

		font::GlyphMetrics getMetricsForGlyph(GlyphId glyph) const;

		// add an explicit mapping from glyph id to a list of codepoints. This is useful for
		// ligature substitutions (eg. 'ffi' -> 'f', 'f', 'i') and for single replacements.
		void addGlyphUnicodeMapping(GlyphId glyph, std::vector<char32_t> codepoints) const;

		font::FontMetrics getFontMetrics() const;

		// get the name that we should put in the Resource dictionary of a page that uses this font.
		std::string getFontResourceName() const;

		GlyphSpace1d scaleFontMetricForPDFGlyphSpace(font::FontScalar metric) const
		{
			return GlyphSpace1d((metric / this->getFontMetrics().units_per_em).value());
		}

		// abstracts away the scaling by units_per_em, to go from font units to pdf units
		// this converts the metric to a **concrete size** (in pdf units, aka 1/72 inches)
		Scalar scaleMetricForFontSize(font::FontScalar metric, Scalar font_size) const
		{
			auto gs = this->scaleFontMetricForPDFGlyphSpace(metric);
			return Scalar((gs * font_size.value()).value());
		}

		// this converts the metric to an **abstract size**, which is the text space. when
		// drawing text, the /Tf directive already specifies the font scale!
		TextSpace1d scaleMetricForPDFTextSpace(font::FontScalar metric) const
		{
			auto gs = this->scaleFontMetricForPDFGlyphSpace(metric);
			return gs.into<TextSpace1d>();
		}

		TextSpace1d convertPDFScalarToTextSpaceForFontSize(Scalar scalar, Scalar font_size)
		{
			return GlyphSpace1d(scalar / font_size).into<TextSpace1d>();
		}

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
		mutable std::map<GlyphId, std::vector<char32_t>> m_extra_unicode_mappings {};

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
