// page.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "pdf/page.h"

namespace sap
{
	// TODO: customisable margins
	constexpr Scalar LEFT_MARGIN    = Scalar(15);
	constexpr Scalar RIGHT_MARGIN   = Scalar(15);
	constexpr Scalar TOP_MARGIN     = Scalar(20);
	constexpr Scalar BOTTOM_MARGIN  = Scalar(20);

	Page::Page(Size2d paper_size)
		: m_layout_region(paper_size - Size2d(LEFT_MARGIN, TOP_MARGIN) - Size2d(RIGHT_MARGIN, BOTTOM_MARGIN))
	{
	}

	pdf::Page* Page::render()
	{
		auto page = util::make<pdf::Page>();
		m_layout_region.render(Position(LEFT_MARGIN, TOP_MARGIN), page);

		return page;
	}
}
