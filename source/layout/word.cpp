// word.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "types.h" // for GlyphId

#include "pdf/font.h"  // for Font, Font::ENCODING_CID
#include "pdf/text.h"  // for Text
#include "pdf/units.h" // for Scalar, pdf_typographic_unit_1d

#include "sap/style.h"    // for Style
#include "sap/units.h"    // for Scalar
#include "sap/fontset.h"  // for FontSet
#include "sap/document.h" // for Word::GlyphInfo, Word, Size2d

#include "font/tag.h"      // for Tag
#include "font/font.h"     // for GlyphMetrics, FontMetrics
#include "font/scalar.h"   // for font_design_space, FontScalar
#include "font/features.h" // for GlyphAdjustment, FeatureSet

namespace sap::layout
{
	Word::Word(zst::wstr_view text, const Style* style) : m_text(text)
	{
		this->setStyle(style);

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
		for(auto& glyph : font->getGlyphInfosForString(m_text))
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
