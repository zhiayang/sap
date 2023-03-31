// spacer.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/spacer.h"

namespace sap::layout
{
	Spacer::Spacer(const Style* style, LayoutSize size) : LayoutObject(style, std::move(size))
	{
	}

	layout::PageCursor Spacer::positionChildren(layout::PageCursor cursor)
	{
		this->positionRelatively(cursor.position());

		if(m_layout_size.descent != 0)
			cursor = cursor.newLine(m_layout_size.descent);

		if(m_layout_size.width != 0)
			cursor = cursor.moveRight(m_layout_size.width);

		return cursor;
	}

	void Spacer::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		(void) layout;
		(void) pages;
	}
}
