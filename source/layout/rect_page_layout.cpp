// rect_page_layout.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/page.h" // for Page

#include "layout/base.h" // for RectPageLayout, LayoutObject

namespace sap::layout
{
	std::vector<pdf::Page*> RectPageLayout::render() const
	{
		std::vector<pdf::Page*> ret;
		for(size_t i = 0; i < m_num_pages; ++i)
		{
			// TODO: Pages should have the size determined by page size
			ret.emplace_back(util::make<pdf::Page>());
		}
		for(auto obj : m_objects)
		{
			obj->render(this, ret);
		}
		return ret;
	}
}
