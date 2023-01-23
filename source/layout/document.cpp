// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/file.h"  // for File
#include "pdf/font.h"  // for Font
#include "pdf/units.h" // for PdfScalar

#include "font/font_file.h"

#include "sap/style.h"       // for Style
#include "sap/units.h"       // for Length
#include "sap/font_family.h" // for FontSet, FontStyle, FontStyle::Regular

#include "tree/document.h"
#include "tree/container.h"

#include "interp/interp.h"
#include "interp/basedefs.h" // for DocumentObject

#include "layout/base.h"      // for Cursor, LayoutObject, Interpreter, Rec...
#include "layout/image.h"     //
#include "layout/document.h"  // for Document
#include "layout/paragraph.h" // for Paragraph

namespace sap::layout
{
	Document::Document(interp::Interpreter* cs) : m_page_layout(PageLayout(dim::mm(210, 297).into<Size2d>(), dim::mm(25)))
	{
		static auto default_font_family = sap::FontFamily(                                //
		    &cs->addLoadedFont(pdf::PdfFont::fromBuiltin(pdf::BuiltinFont::TimesRoman)),  //
		    &cs->addLoadedFont(pdf::PdfFont::fromBuiltin(pdf::BuiltinFont::TimesItalic)), //
		    &cs->addLoadedFont(pdf::PdfFont::fromBuiltin(pdf::BuiltinFont::TimesBold)),   //
		    &cs->addLoadedFont(pdf::PdfFont::fromBuiltin(pdf::BuiltinFont::TimesBoldItalic)));

		// TODO: set root font size based on some preamble
		static auto default_style = //
		    sap::Style()
		        .set_font_family(default_font_family)
		        .set_font_style(sap::FontStyle::Regular)
		        .set_font_size(pdf::PdfScalar(12.0).into())
		        .set_root_font_size(pdf::PdfScalar(12.0).into())
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

	void Document::write(pdf::Writer* stream)
	{
		auto pages = m_page_layout.render();
		for(auto& page : pages)
			m_pdf_document.addPage(page);

		m_pdf_document.write(stream);
	}
}

namespace sap::tree
{
	void Document::layout(interp::Interpreter* cs, layout::Document* layout_doc)
	{
		auto cursor = layout_doc->pageLayout().newCursor();
		auto objs = m_container->createLayoutObject(cs, cursor, layout_doc->style());

		for(auto& obj : objs.objects)
			layout_doc->pageLayout().addObject(std::move(obj));
	}


	void Document::addObject(std::unique_ptr<BlockObject> obj)
	{
		m_container->contents().push_back(std::move(obj));
	}

	std::unique_ptr<tree::VertBox> Document::takeContainer() &&
	{
		return std::move(m_container);
	}

	Document::Document() : m_container(std::make_unique<tree::VertBox>())
	{
	}

	BlockObject::~BlockObject()
	{
	}
}
