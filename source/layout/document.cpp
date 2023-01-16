// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/file.h"  // for File
#include "pdf/font.h"  // for Font
#include "pdf/units.h" // for PdfScalar

#include "font/font_file.h"

#include "sap/style.h"       // for Style
#include "sap/units.h"       // for Length
#include "sap/document.h"    // for Document
#include "sap/font_family.h" // for FontSet, FontStyle, FontStyle::Regular

#include "tree/document.h"

#include "interp/basedefs.h" // for DocumentObject

#include "layout/base.h"      // for Cursor, LayoutObject, Interpreter, Rec...
#include "layout/image.h"     //
#include "layout/paragraph.h" // for Paragraph

namespace sap::layout
{
	Document::Document() : m_page_layout(PageLayout(dim::mm(210, 297).into<Size2d>(), dim::mm(25)))
	{
		static auto default_font_family = sap::FontFamily(                       //
		    this->addFont(pdf::BuiltinFont::get(pdf::BuiltinFont::TimesRoman)),  //
		    this->addFont(pdf::BuiltinFont::get(pdf::BuiltinFont::TimesItalic)), //
		    this->addFont(pdf::BuiltinFont::get(pdf::BuiltinFont::TimesBold)),   //
		    this->addFont(pdf::BuiltinFont::get(pdf::BuiltinFont::TimesBoldItalic)));

		static auto default_style = //
		    sap::Style()
		        .set_font_family(default_font_family)
		        .set_font_style(sap::FontStyle::Regular)
		        .set_font_size(pdf::PdfScalar(12.0).into<sap::Length>())
		        .set_line_spacing(1.0)
		        .set_alignment(Alignment::Justified)
		        .set_paragraph_spacing(sap::Length(1.0));

		this->setStyle(&default_style);
	}

	pdf::File& Document::pdf()
	{
		return m_pdf_document;
	}

	const pdf::File& Document::pdf() const
	{
		return m_pdf_document;
	}

	void Document::addObject(std::unique_ptr<LayoutObject> obj)
	{
		m_objects.push_back(std::move(obj));
	}

	pdf::PdfFont* Document::addFont(std::unique_ptr<font::FontSource> font)
	{
		auto f = pdf::PdfFont::fromSource(std::move(font));
		return m_fonts.emplace_back(std::move(f)).get();
	}

	void Document::layout(interp::Interpreter* cs, const tree::Document& treedoc)
	{
		LineCursor cursor = m_page_layout.newCursor();
		for(const auto& obj : treedoc.objects())
		{
			auto [new_cursor, layout_obj] = obj->createLayoutObject(cs, cursor, m_style);
			cursor = std::move(new_cursor);

			if(layout_obj.has_value())
				m_page_layout.addObject(std::move(*layout_obj));
		}
	}

	void Document::write(pdf::Writer* stream)
	{
		auto pages = m_page_layout.render();
		for(auto& page : pages)
			m_pdf_document.addPage(page);

		m_pdf_document.write(stream);
	}
}
