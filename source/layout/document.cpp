// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "util.h"
#include "pdf/page.h"
#include "pdf/document.h"
#include "interp/tree.h"

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
		        .set_font_size(pdf::Scalar(12.0).into(sap::Scalar {}))
		        .set_line_spacing(sap::Scalar(1.0))
		        .set_pre_paragraph_spacing(sap::Scalar(1.0))
		        .set_post_paragraph_spacing(sap::Scalar(1.0));

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
		m_pages.emplace_back(dim::Vector2(dim::mm(210), dim::mm(297)).into(Size2d {}));

		for(const auto& obj : treedoc.objects())
		{
			if(auto treepara = util::dynamic_pointer_cast<tree::Paragraph>(obj); treepara != nullptr)
			{
				std::optional<const tree::Paragraph*> overflow = treepara.get();
				while(true)
				{
					overflow = Paragraph::layout(cs, m_pages.back().layoutRegion(), m_style, *overflow);
					if(overflow)
					{
						m_pages.emplace_back(dim::Vector2(dim::mm(210), dim::mm(297)).into(Size2d {}));
					}
					else
					{
						break;
					}
				}
			}
			else
				sap::internal_error("lol");
		}
	}

	void Document::render()
	{
		for(auto& page : m_pages)
			m_pdf_document.addPage(page.render());
	}

}
