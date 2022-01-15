// page.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "pdf/page.h"

namespace sap
{
	// TODO: compute margins
	Page::Page(Size2d paper_size) : m_layout_region(paper_size)
	{
	}

	pdf::Page* Page::render()
	{
		auto page = util::make<pdf::Page>();
		m_layout_region.render(Position(10, 10), page);

		return page;
	}
}
