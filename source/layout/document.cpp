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

	void Document::add(Page&& para)
	{
		m_pages.emplace_back(std::move(para));
	}

	pdf::Document Document::finalise()
	{
		for(auto& page : m_pages)
			m_pdf_document.addPage(page.finalise());

		return std::move(m_pdf_document);
	}
}
