// spacer.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "layout/layout_object.h"

namespace sap::layout
{
	struct LayoutBase;

	struct Spacer : LayoutObject
	{
		explicit Spacer(const Style* cur_style, LayoutSize size);

		virtual layout::PageCursor positionChildren(layout::PageCursor cursor) override;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;
	};
}
