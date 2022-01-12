// page.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "pdf/page.h"

namespace sap
{
	Page::Page()
	{
		this->pdf_page = util::make<pdf::Page>();
	}

	std::optional<Paragraph> Page::add(Paragraph para)
	{
		return { };
	}
}
