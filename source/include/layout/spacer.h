// spacer.h
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "layout/layout_object.h"

namespace sap::layout
{
	struct LayoutBase;

	struct Spacer : LayoutObject
	{
		explicit Spacer(const Style& cur_style, LayoutSize size);

		virtual bool is_phantom() const override;
		virtual layout::PageCursor compute_position_impl(layout::PageCursor cursor) override;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;
	};
}
