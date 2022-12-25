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

	/* static std::unique_ptr<Paragraph> layoutParagraph(interp::Interpreter* cs, const std::shared_ptr<tree::Paragraph>& para) */
	/* { */
	/* 	auto ret = std::make_unique<Paragraph>(); */

	/* 	for(auto& obj : para->contents()) */
	/* 	{ */
	/* 		if(auto txt = std::dynamic_pointer_cast<tree::Text>(obj); txt != nullptr) */
	/* 		{ */
	/* 			ret->add(Text::fromTreeText(*txt)); */
	/* 			ret->add(Word::fromTreeText(*txt)); */
	/* 		} */

	/* 		else if(auto sep = std::dynamic_pointer_cast<tree::Separator>(obj); sep != nullptr) */
	/* 			ret->add(Text::separator()); */

	/* 		else */
	/* 			sap::internal_error("coeu"); */
	/* 	} */

	/* 	return ret; */
	/* } */

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

#if 0
		for(const auto& obj : treedoc.objects())
		{
			if(auto para = std::dynamic_pointer_cast<tree::Paragraph>(obj); para != nullptr)
			{
				addObject(layoutParagraph(cs, para));
			}
			else if(auto scr = std::dynamic_pointer_cast<tree::ScriptObject>(obj); scr != nullptr)
			{
				// TODO
			}
		}


		if(m_objects.empty())
			return;

		LayoutObject* overflow = nullptr;
		for(size_t i = 0; i < m_objects.size();)
		{
			if(m_pages.empty() || overflow != nullptr)
				m_pages.emplace_back(dim::Vector2(dim::mm(210), dim::mm(297)).into(Size2d {}));

			auto page = &m_pages.back();
			auto obj = (overflow == nullptr ? m_objects[i].get() : overflow);

			// TODO: handle this more elegantly (we should try to move this to the next page)
			if(auto result = obj->layout(cs, page->layoutRegion(), m_style); !result.ok())
			{
				sap::error("layout/page", "page layout failed");
			}
			else if(result->has_value())
			{
				overflow = result->value();
			}
			else
			{
				overflow = nullptr;
				i++;
			}
		}
#endif
	}

	void Document::render()
	{
		for(auto& page : m_pages)
			m_pdf_document.addPage(page.render());
	}

}
