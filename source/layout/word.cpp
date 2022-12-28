// word.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "types.h" // for GlyphId

#include "pdf/font.h"  // for Font, Font::ENCODING_CID
#include "pdf/text.h"  // for Text
#include "pdf/units.h" // for Scalar

#include "sap/style.h"   // for Style
#include "sap/units.h"   // for Scalar
#include "sap/fontset.h" // for FontSet

#include "font/font.h"     // for GlyphInfo
#include "font/features.h" // for GlyphAdjustment

#include "layout/word.h"

namespace sap::layout
{
	Word::Word(zst::wstr_view text, const Style* style) : m_text(text)
	{
		this->setStyle(style);

		assert(m_style != nullptr);
	}


	void Word::render(pdf::Text* text, Length space) const
	{
		const auto font = m_style->font();
		const auto font_size = m_style->font_size();
		text->setFont(font, font_size.into<pdf::PdfScalar>());

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
