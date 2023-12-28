// path.cpp
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/path.h"

namespace sap::layout
{
	Path::Path(const Style& style, LayoutSize size, std::shared_ptr<PathSegments> segments)
	    : LayoutObject(style, size), m_segments(std::move(segments))
	{
	}

	bool Path::requires_space_reservation() const
	{
		return true;
	}

	layout::PageCursor Path::compute_position_impl(layout::PageCursor cursor)
	{
		return cursor;
	}

	void Path::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
	}
}
