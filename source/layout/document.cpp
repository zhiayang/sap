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

	}

	pdf::Document Document::render()
	{
		for(auto& page : m_pages)
			m_pdf_document.addPage(page.render());

		return std::move(m_pdf_document);
	}
}
