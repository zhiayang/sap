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
	Word::Word(zst::wstr_view text, const Style* style, Length relative_offset, LayoutSize size)
		: LayoutObject(style, size), m_relative_offset(relative_offset), m_text(text)
	{
		assert(m_style != nullptr);
	}

	void Word::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		sap::internal_error("words cannot be rendered directly");
	}

	layout::PageCursor Word::positionChildren(layout::PageCursor cursor)
	{
		sap::internal_error("words cannot be positioned directly");
		return cursor;
	}


	void Word::render(AbsolutePagePos line_pos,
		std::vector<pdf::Page*>& pages,
		pdf::Text* text,
		bool is_first_in_text,
		Length offset_from_prev) const
	{
		auto page = pages[line_pos.page_num];

		if(not is_first_in_text)
		{
			text->offset(pdf::PdfFont::convertPDFScalarToTextSpaceForFontSize(offset_from_prev.into(),
				text->currentFontSize()));
		}
		else
		{
			text->moveAbs(page->convertVector2(line_pos.pos.into()));
		}

		this->render_to_text(text);

		if(is_first_in_text)
			page->addObject(text);
	}

	void Word::render_to_text(pdf::Text* text) const
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
