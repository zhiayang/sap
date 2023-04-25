// word.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <utf8proc/utf8proc.h>

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
	Word::Word(std::u32string text, const Style& style, Length relative_offset, Length raise_height, LayoutSize size)
	    : LayoutObject(style, size)
	    , m_relative_offset(relative_offset)
	    , m_raise_height(raise_height)
	    , m_text(std::move(text))
	{
	}

	void Word::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		sap::internal_error("words cannot be rendered directly");
	}

	layout::PageCursor Word::compute_position_impl(layout::PageCursor cursor)
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
			auto pos = line_pos.pos;
			if(offset_from_prev != 0)
				pos.x() += offset_from_prev;

			text->moveAbs(page->convertVector2(pos.into()));
		}

		this->render_to_text(text);

		if(is_first_in_text)
			page->addObject(text);
	}

	void Word::render_to_text(pdf::Text* text) const
	{
		const auto font = m_style.font();
		const auto font_size = m_style.font_size();
		text->setFont(font, font_size.into<pdf::PdfScalar>());
		text->setColour(m_style.colour());

		if(m_raise_height != 0)
			text->rise(m_raise_height.into());

		for(auto& glyph : font->getGlyphInfosForString(m_text))
		{
			auto placement = font->scaleMetricForPDFTextSpace(glyph.adjustments.horz_placement);

			// FIXME: not sure if this is correct, but it works for our purposes.
			// adjust by placement, put the glyph, then adjust back...
			text->offset(placement);

#if 1
			text->addEncoded(font->isCIDFont() ? 2 : 1, static_cast<uint32_t>(glyph.gid));
#else
			// note: UTF-8 encoding is broken, see the note in pdf_font.cpp
			auto codepoint = font->getOutputCodepointForGlyph(glyph.gid);
			text->addUnicodeText(zst::wstr_view(&codepoint, 1));
#endif
			text->offset(-placement);

			// TODO: handle placement as well
			text->offset(font->scaleMetricForPDFTextSpace(glyph.adjustments.horz_advance));
		}

		if(m_raise_height != 0)
			text->rise(0);
	}
}
