// font.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"  // for hashmap
#include "types.h" // for GlyphId

#include "pdf/units.h" // for TextSpace1d, PdfScalar, GlyphSpace1d
#include "pdf/builtin_font.h"

#include "font/metrics.h"
#include "font/features.h"
#include "font/font_scalar.h" // for font_design_space, FontScalar
#include "font/font_source.h"

namespace font
{
	struct FontFile;
}

namespace pdf
{
	struct Array;
	struct Stream;
	struct File;
	struct Dictionary;

	struct PdfFont
	{
		Dictionary* serialise(File* doc) const;
		bool didSerialise() const { return m_did_serialise; }
		Dictionary* dictionary() const { return m_font_dictionary; }
		std::string getFontResourceName() const { return m_font_resource_name; }
		bool isCIDFont() const { return not m_source->isBuiltin(); }

		const font::FontMetrics& getFontMetrics() const { return m_source->metrics(); }
		font::GlyphMetrics getMetricsForGlyph(GlyphId glyph) const { return m_source->getGlyphMetrics(glyph); }
		GlyphId getGlyphIdFromCodepoint(char32_t codepoint) const { return m_source->getGlyphIndexForCodepoint(codepoint); }

		std::vector<font::GlyphInfo> getGlyphInfosForString(zst::wstr_view text) const
		{
			return m_source->getGlyphInfosForString(text);
		}

		Size2d_YDown getWordSize(zst::wstr_view text, PdfScalar font_size) const;

		// A very thin wrapper around the identically-named methods taking a FontFile
		std::map<size_t, font::GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(zst::span<GlyphId> glyphs,
		    const font::FeatureSet& features) const;

		std::optional<std::vector<GlyphId>> performSubstitutionsForGlyphSequence(zst::span<GlyphId> glyphs,
		    const font::FeatureSet& features) const;


		// add an explicit mapping from glyph id to a list of codepoints. This is useful for
		// ligature substitutions (eg. 'ffi' -> 'f', 'f', 'i') and for single replacements.
		void addGlyphUnicodeMapping(GlyphId glyph, std::vector<char32_t> codepoints) const;

		GlyphSpace1d scaleFontMetricForPDFGlyphSpace(font::FontScalar metric) const
		{
			return GlyphSpace1d((metric / this->getFontMetrics().units_per_em).value());
		}

		// abstracts away the scaling by units_per_em, to go from font units to pdf units
		// this converts the metric to a **concrete size** (in pdf units, aka 1/72 inches)
		PdfScalar scaleMetricForFontSize(font::FontScalar metric, PdfScalar font_size) const
		{
			auto gs = this->scaleFontMetricForPDFGlyphSpace(metric);
			return PdfScalar((gs * font_size.value()).value());
		}

		// this converts the metric to an **abstract size**, which is the text space. when
		// drawing text, the /Tf directive already specifies the font scale!
		TextSpace1d scaleMetricForPDFTextSpace(font::FontScalar metric) const
		{
			auto gs = this->scaleFontMetricForPDFGlyphSpace(metric);
			return gs.into<TextSpace1d>();
		}

		int font_type = 0;

		static PdfFont* fromBuiltin(File* doc, BuiltinFont::Core14 font_name);
		static PdfFont* fromFontFile(File* doc, std::unique_ptr<font::FontFile> font);

		static constexpr int FONT_TYPE1 = 1;
		static constexpr int FONT_TRUETYPE = 2;
		static constexpr int FONT_CFF_CID = 3;
		static constexpr int FONT_TRUETYPE_CID = 4;

		static TextSpace1d convertPDFScalarToTextSpaceForFontSize(PdfScalar scalar, PdfScalar font_size)
		{
			return GlyphSpace1d(scalar / font_size).into<TextSpace1d>();
		}

	private:
		PdfFont(File* doc, BuiltinFont::Core14 builtin_font);
		PdfFont(File* doc, std::unique_ptr<font::FontFile> font);

		void writeUnicodeCMap(File* doc) const;
		void writeCIDSet(File* doc) const;

		mutable std::map<GlyphId, std::vector<char32_t>> m_extra_unicode_mappings {};
		mutable bool m_did_serialise = false;

		std::unique_ptr<font::FontSource> m_source {};
		Dictionary* m_font_dictionary = nullptr;
		Array* m_glyph_widths_array = nullptr;

		// the name that goes into the Resource << >> dict in a page. This is a unique name
		// that we get from the Document when the font is created.
		std::string m_font_resource_name {};



		Stream* m_embedded_contents = nullptr;
		Stream* m_unicode_cmap = nullptr;
		Stream* m_cidset = nullptr;

		// what goes in BaseName. for subsets, this includes the ABCDEF+ part.
		std::string m_pdf_font_name;

		// pool needs to be a friend because it needs the constructor
		template <typename>
		friend struct util::Pool;
	};
}
