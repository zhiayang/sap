// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "pdf/page.h"
#include "pdf/document.h"

namespace sap::layout
{
	Document::Document()
	{
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

	void Document::layout(interp::Interpreter* cs)
	{
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
	}

	pdf::Document& Document::render(interp::Interpreter* cs)
	{
		for(auto& page : m_pages)
			m_pdf_document.addPage(page.render(cs));

		return m_pdf_document;
	}

}
