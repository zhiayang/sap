// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/font.h"     // for Font
#include "pdf/units.h"    // for PdfScalar
#include "pdf/document.h" // for File

#include "font/font_file.h"

#include "sap/style.h"       // for Style
#include "sap/units.h"       // for Length
#include "sap/document.h"    // for Document
#include "sap/font_family.h" // for FontSet, FontStyle, FontStyle::Regular

#include "interp/tree.h"     // for Paragraph, Document
#include "interp/basedefs.h" // for DocumentObject

#include "layout/base.h"      // for Cursor, LayoutObject, Interpreter, Rec...
#include "layout/paragraph.h" // for Paragraph

namespace sap::layout
{
	Document::Document()
	{
		static auto default_font_family = sap::FontFamily(                       //
		    this->addFont(pdf::BuiltinFont::get(pdf::BuiltinFont::TimesRoman)),  //
		    this->addFont(pdf::BuiltinFont::get(pdf::BuiltinFont::TimesItalic)), //
		    this->addFont(pdf::BuiltinFont::get(pdf::BuiltinFont::TimesBold)),   //
		    this->addFont(pdf::BuiltinFont::get(pdf::BuiltinFont::TimesBoldItalic)));

		static auto default_style =
		    sap::Style()
		        .set_font_family(default_font_family)
		        .set_font_style(sap::FontStyle::Regular)
		        .set_font_size(pdf::PdfScalar(12.0).into<sap::Length>())
		        .set_line_spacing(sap::Length(1.0))
		        .set_pre_paragraph_spacing(sap::Length(1.0))
		        .set_post_paragraph_spacing(sap::Length(1.0));

		setStyle(&default_style);
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
		auto f = pdf::PdfFont::fromSource(&m_pdf_document, std::move(font));
		return m_fonts.emplace_back(std::move(f)).get();
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
