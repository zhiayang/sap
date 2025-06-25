// path.h
// Copyright (c) 2023, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/path.h"
#include "sap/style.h"
#include "sap/units.h"

#include "layout/base.h"

namespace pdf
{
	struct Text;
}

namespace sap::layout
{
	struct Path : LayoutObject
	{
		Path(const Style& style,
		    LayoutSize size,
		    PathStyle path_style,
		    std::shared_ptr<PathSegments> segments,
		    Position origin);

		virtual bool requires_space_reservation() const override;
		virtual layout::PageCursor compute_position_impl(layout::PageCursor cursor) override;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

		const PathSegments& segments() const { return *m_segments; }

	private:
		PathStyle m_path_style;
		std::shared_ptr<PathSegments> m_segments;
		Position m_origin;
	};
}
