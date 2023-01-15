// word.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "types.h" // for GlyphId

#include "pdf/font.h"  // for Font, Font::ENCODING_CID
#include "pdf/text.h"  // for Text
#include "pdf/page.h"  //
#include "pdf/units.h" // for PdfScalar

#include "sap/style.h" // for Style
#include "sap/units.h" // for Length

#include "font/features.h"  // for GlyphAdjustment
#include "font/font_file.h" // for GlyphInfo

#include "layout/word.h" // for Word

namespace sap::layout
{
	Word::Word(zst::wstr_view text, const Style* style, RelativePos pos, Size2d size) //
	    : LayoutObject(pos, size)
	    , m_text(text)
	{
		this->setStyle(style);

		assert(m_style != nullptr);
	}

	void Word::render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		auto text = util::make<pdf::Text>();

		auto pos = layout->convertPosition(m_layout_position);
		auto page = pages[pos.page_num];
		text->moveAbs(page->convertVector2(pos.pos.into()));

		this->line_render(text);
		page->addObject(text);
	}

	void Word::line_render(pdf::Text* text) const
	{
		const auto font = m_style->font();
		const auto font_size = m_style->font_size();
		text->setFont(font, font_size.into<pdf::PdfScalar>());

		auto add_gid = [&font, text](GlyphId gid) {
			if(font->isCIDFont())
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
	}
}
