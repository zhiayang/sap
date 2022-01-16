// document.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "pdf/page.h"
#include "pdf/document.h"

namespace sap
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

	void Document::addObject(LayoutObject* obj)
	{
		m_objects.push_back(obj);
	}

	void Document::layout()
	{
		if(m_objects.empty())
			return;

		LayoutObject* overflow = nullptr;
		for(size_t i = 0; i < m_objects.size();)
		{
			if(m_pages.empty())
				m_pages.emplace_back(dim::Vector2(dim::mm(180), dim::mm(297)).into(Size2d{}));

			auto page = &m_pages.back();
			auto obj = (overflow == nullptr ? m_objects[i] : overflow);

			// TODO: handle this more elegantly (we should try to move this to the next page)
			if(auto result = obj->layout(page->layoutRegion(), m_style); !result.ok())
			{
				sap::error("layout/page", "page layout failed");
			}
			else if(result->has_value())
			{
				overflow = result->value();
			}
			else
			{
				i++;
			}
		}
	}

	pdf::Document& Document::render()
	{
		for(auto& page : m_pages)
			m_pdf_document.addPage(page.render());

		return m_pdf_document;
	}
}
