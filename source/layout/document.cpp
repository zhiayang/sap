// document.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "pdf/document.h"

namespace sap
{
	Document::Document() : m_pdf_document()
	{
	}

	void Document::add(Page&& para)
	{
		m_pages.emplace_back(std::move(para));
	}

	void Document::finalise(pdf::Writer* writer)
	{
	}
}
