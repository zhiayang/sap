// font.h
// Copyright (c) 2021, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"  // for hashmap
#include "types.h" // for GlyphId

#include "pdf/units.h" // for TextSpace1d, PdfScalar, GlyphSpace1d
#include "pdf/resource.h"
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
	struct Object;
	struct Dictionary;

	struct PdfFont : Resource
	{
		using GlyphPosAdjMap = util::hashmap<size_t, font::GlyphAdjustment>;

		virtual Object* resourceObject() const override;
		virtual void serialise() const override;

		Dictionary* dictionary() const { return m_font_dictionary; }
		bool isCIDFont() const { return not m_source->isBuiltin(); }

		const font::FontSource& source() const { return *m_source; }

		const font::FontMetrics& getFontMetrics() const { return m_source->metrics(); }
		font::GlyphMetrics getMetricsForGlyph(GlyphId glyph) const { return m_source->getGlyphMetrics(glyph); }
		GlyphId getGlyphIdFromCodepoint(char32_t codepoint) const
		{
			return m_source->getGlyphIndexForCodepoint(codepoint);
		}

		int64_t fontId() const { return m_font_id; }

		const std::vector<font::GlyphInfo>& getGlyphInfosForString(zst::wstr_view text) const;

		Size2d_YDown getWordSize(zst::wstr_view text, PdfScalar font_size) const;

		// returns the GlyphInfos for the given glyph string. note that this *DOES NOT* perform GSUB/morx, ie.
		// ligatures and/or language-specific glyphs are not done -- hence 'substituted string'
		std::vector<font::GlyphInfo>
		getGlyphInfosForSubstitutedString(zst::span<GlyphId> glyphs, const font::FeatureSet& features) const;

		std::optional<std::vector<GlyphId>>
		performSubstitutionsForGlyphSequence(zst::span<GlyphId> glyphs, const font::FeatureSet& features) const;

		// add an explicit mapping from glyph id to a list of codepoints. This is useful for
		// ligature substitutions (eg. 'ffi' -> 'f', 'f', 'i') and for single replacements.
		void addGlyphUnicodeMapping(GlyphId glyph, std::vector<char32_t> codepoints) const;

		GlyphSpace1d scaleFontMetricForPDFGlyphSpace(font::FontScalar metric) const;

		// abstracts away the scaling by units_per_em, to go from font units to pdf units
		// this converts the metric to a **concrete size** (in pdf units, aka 1/72 inches)
		PdfScalar scaleMetricForFontSize(font::FontScalar metric, PdfScalar font_size) const;

		// this converts the metric to an **abstract size**, which is the text space. when
		// drawing text, the /Tf directive already specifies the font scale!
		TextSpace1d scaleMetricForPDFTextSpace(font::FontScalar metric) const;

		char32_t getOutputCodepointForGlyph(GlyphId glyph) const;

		void addAdditionalGlyphPositioningAdjustment(std::vector<GlyphId> gids, GlyphPosAdjMap adjustments);

		int font_type = 0;

		static std::unique_ptr<PdfFont> fromBuiltin(BuiltinFont::Core14 font_name);
		static std::unique_ptr<PdfFont> fromSource(std::unique_ptr<font::FontSource> font);

		static constexpr int FONT_TYPE1 = 1;
		static constexpr int FONT_TRUETYPE = 2;
		static constexpr int FONT_CFF_CID = 3;
		static constexpr int FONT_TRUETYPE_CID = 4;

		static TextSpace1d convertPDFScalarToTextSpaceForFontSize(PdfScalar scalar, PdfScalar font_size);

	private:
		PdfFont(std::unique_ptr<pdf::BuiltinFont> builtin_font);
		PdfFont(std::unique_ptr<font::FontFile> font);

		void writeUnicodeCMap() const;
		void writeUTF8CMap() const;
		void writeCIDSet() const;

		mutable util::hashmap<std::u32string, font::FontVector2d> m_word_size_cache {};
		mutable util::hashmap<std::u32string, std::vector<font::GlyphInfo>> m_glyph_infos_cache {};

		mutable util::hashmap<GlyphId, std::vector<char32_t>> m_extra_unicode_mappings {};

		mutable util::hashmap<GlyphId, char32_t> m_extra_glyph_to_private_use_mapping {};
		mutable char32_t m_cur_unicode_private_use_codepoint = 0;

		mutable bool m_did_serialise = false;

		// map from the first character of the adjustment to the actual list of adjustments
		util::hashmap<GlyphId, std::vector<std::pair<std::vector<GlyphId>, GlyphPosAdjMap>>>
		    m_custom_glyph_pos_adjustments {};

		std::unique_ptr<font::FontSource> m_source {};
		Dictionary* m_font_dictionary = nullptr;
		Array* m_glyph_widths_array = nullptr;

		Stream* m_embedded_contents = nullptr;
		Stream* m_tounicode_cmap = nullptr;
		Stream* m_utf8_cmap = nullptr;
		Stream* m_cidset = nullptr;

		// what goes in BaseName. for subsets, this includes the ABCDEF+ part.
		std::string m_pdf_font_name;

		int64_t m_font_id;

		// pool needs to be a friend because it needs the constructor
		template <typename, size_t>
		friend struct util::Pool;
	};
}
