// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for dynamic_pointer_cast

#include "pdf/font.h"     // for Font
#include "pdf/units.h"    // for Scalar
#include "pdf/document.h" // for Document

#include "sap/style.h"    // for Style
#include "sap/units.h"    // for Scalar
#include "sap/fontset.h"  // for FontSet, FontStyle, FontStyle::Regular
#include "sap/document.h" // for Document, Cursor, LayoutObject, Interpreter

#include "interp/tree.h" // for Document, Paragraph

#include "layout/paragraph.h"

namespace sap::layout
{
	Document::Document()
	{
		static auto default_font_set = sap::FontSet( //
		    pdf::Font::fromBuiltin(&pdfDocument(), "Times-Roman"), pdf::Font::fromBuiltin(&pdfDocument(), "Times-Italic"),
		    pdf::Font::fromBuiltin(&pdfDocument(), "Times-Bold"), pdf::Font::fromBuiltin(&pdfDocument(), "Times-BoldItalic"));

		static auto default_style =
		    sap::Style()
		        .set_font_set(default_font_set)
		        .set_font_style(sap::FontStyle::Regular)
		        .set_font_size(pdf::PdfScalar(12.0).into<sap::Length>())
		        .set_line_spacing(sap::Length(1.0))
		        .set_pre_paragraph_spacing(sap::Length(1.0))
		        .set_post_paragraph_spacing(sap::Length(1.0));

		setStyle(&default_style);
	}

	pdf::Document& Document::pdfDocument()
	{
		return m_pdf_document;
	}

	const pdf::Document& Document::pdfDocument() const
	{
		return m_pdf_document;
	}

	void Document::addObject(std::unique_ptr<LayoutObject> obj)
	{
		m_objects.push_back(std::move(obj));
	}

	void Document::layout(interp::Interpreter* cs, const tree::Document& treedoc)
	{
		Cursor cursor = m_page_layout.newCursor();
		for(const auto& obj : treedoc.objects())
		{
			if(auto treepara = dynamic_cast<const tree::Paragraph*>(obj); treepara != nullptr)
			{
				std::optional<const tree::Paragraph*> overflow = treepara;
				while(overflow)
					std::tie(overflow, cursor) = Paragraph::layout(cs, &m_page_layout, cursor, m_style, *overflow);
			}
			else
			{
				sap::internal_error("lol");
			}
		}
	}

}
