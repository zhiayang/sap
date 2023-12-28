// path.h
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/style.h"
#include "sap/units.h"
#include "sap/path_object.h"

#include "layout/base.h"

namespace pdf
{
	struct Text;
}

namespace sap::layout
{
	struct Path : LayoutObject
	{
		Path(const Style& style, LayoutSize size, std::shared_ptr<PathObjects> segments);

		virtual bool requires_space_reservation() const override;
		virtual layout::PageCursor compute_position_impl(layout::PageCursor cursor) override;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

		const PathObjects& segments() const { return *m_segments; }

	private:
		std::shared_ptr<PathObjects> m_segments;
	};
}
