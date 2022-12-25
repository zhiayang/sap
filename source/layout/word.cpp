// word.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/units.h"
#include "sap.h"
#include "util.h"

#include "font/font.h"

#include "pdf/pdf.h"
#include "pdf/text.h"
#include "pdf/misc.h"
#include "pdf/font.h"

#include "interp/tree.h"

namespace sap::layout
{
	// TODO: this needs to handle unicode composing/decomposing also, which is a massive pain
	static GlyphId read_one_glyphid(const pdf::Font* font, zst::byte_span& utf8)
	{
		assert(utf8.size() > 0);

		auto cp = unicode::consumeCodepointFromUtf8(utf8);
		return font->getGlyphIdFromCodepoint(cp);
	}


	static std::vector<Word::GlyphInfo> convert_to_glyphs(const pdf::Font* font, zst::str_view text)
	{
		auto utf8 = text.bytes();

		// first, convert all codepoints to glyphs
		std::vector<GlyphId> glyphs {};
		while(utf8.size() > 0)
			glyphs.push_back(read_one_glyphid(font, utf8));

		using font::Tag;
		font::off::FeatureSet features {};

		// REMOVE: this is just for testing!
		features.script = Tag("cyrl");
		features.language = Tag("BGR ");
		features.enabled_features = { Tag("kern"), Tag("liga"), Tag("locl") };

		auto span = [](auto& foo) {
			return zst::span<GlyphId>(foo.data(), foo.size());
		};

		// next, use GSUB to perform substitutions.
		glyphs = font->performSubstitutionsForGlyphSequence(span(glyphs), features);

		// next, get base metrics for each glyph.
		std::vector<Word::GlyphInfo> glyph_infos {};
		for(auto g : glyphs)
		{
			Word::GlyphInfo info {};
			info.gid = g;
			info.metrics = font->getMetricsForGlyph(g);
			glyph_infos.push_back(std::move(info));
		}

		// finally, use GPOS
		auto adjustment_map = font->getPositioningAdjustmentsForGlyphSequence(span(glyphs), features);
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


	Word::Word(zst::str_view text, const Style* style) : m_text(text)
	{
		this->setStyle(style);

		auto font = style->font_set().getFontForStyle(style->font_style());
		auto font_size = style->font_size();

		m_glyphs = convert_to_glyphs(font, m_text);

		// size is in sap units, which is in mm; metrics are in typographic units, so 72dpi;
		// calculate the scale accordingly.
		const auto font_metrics = font->getFontMetrics();
		auto font_size_tpu = font_size.into<pdf::Scalar>();

		m_size = { 0, 0 };
		m_size.y() = font->scaleMetricForFontSize(font_metrics.default_line_spacing, font_size_tpu).into<sap::Scalar>();

		for(auto& glyph : m_glyphs)
		{
			auto width = glyph.metrics.horz_advance + glyph.adjustments.horz_advance;
			m_size.x() += font->scaleMetricForFontSize(width, font_size_tpu).into<sap::Scalar>();
		}

		// this space width is to the next word
		auto space_gid = font->getGlyphIdFromCodepoint(U' ');
		auto space_adv = font->getMetricsForGlyph(space_gid).horz_advance;
		auto space_width = font->scaleMetricForFontSize(space_adv, font_size_tpu);

		m_space_width = space_width.into<sap::Scalar>();

		assert(m_style != nullptr);
	}

	void Word::render(pdf::Text* text, Scalar space) const
	{
		const auto font = m_style->font_set().getFontForStyle(m_style->font_style());
		const auto font_size = m_style->font_size();
		text->setFont(font, font_size.into<pdf::Scalar>());

		auto add_gid = [&font, text](GlyphId gid) {
			if(font->encoding_kind == pdf::Font::ENCODING_CID)
				text->addEncoded(2, static_cast<uint32_t>(gid));
			else
				text->addEncoded(1, static_cast<uint32_t>(gid));
		};

		if(space.nonzero())
		{
			// TODO: render spaces in space::render
			auto space_gid = font->getGlyphIdFromCodepoint(U' ');
			add_gid(space_gid);

			auto emitted_space_adv = font->getMetricsForGlyph(space_gid).horz_advance;
			auto font_size_tpu = font_size.into<pdf::Scalar>();
			auto emitted_space_size = font->scaleMetricForFontSize(emitted_space_adv, font_size_tpu);
			auto desired_space_size = space.into<pdf::Scalar>();

			if(emitted_space_size != desired_space_size)
			{
				auto extra = -emitted_space_size + desired_space_size;
				text->offset(pdf::Font::pdfScalarToTextScalarForFontSize(extra, font_size_tpu));
			}
		}

		for(auto& glyph : m_glyphs)
		{
			add_gid(glyph.gid);

			// TODO: handle placement as well
			text->offset(font->scaleMetricForPDFTextSpace(glyph.adjustments.horz_advance));
		}


		/*
		    TODO: here, we also want to handle kerning between the space and the start of the next word
		    (see the longer explanation in layout/paragraph.cpp)
		*/
	}
}
