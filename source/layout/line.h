// line.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <span>

#include "layout/base.h"

namespace sap::tree
{
	struct Separator;
	struct BlockObject;
	struct InlineObject;
}

namespace sap::layout
{
	namespace linebreak
	{
		struct BrokenLine;
	}

	struct LineAdjustment
	{
		Length left_protrusion;
		Length right_protrusion;
		util::hashmap<size_t, Length> piece_adjustments;
	};

	struct Line : LayoutObject
	{
		virtual layout::PageCursor compute_position_impl(layout::PageCursor cursor) override;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

		static std::unique_ptr<Line> fromInlineObjects(interp::Interpreter* cs,
		    const Style& style,
		    std::span<const zst::SharedPtr<tree::InlineObject>> objs,
		    Size2d available_space,
		    bool is_first_line,
		    bool is_last_line,
		    std::optional<LineAdjustment> adjustment = std::nullopt);

		sap::Length lineSpacing() const { return m_line_spacing; }
		const std::vector<std::unique_ptr<LayoutObject>>& objects() const { return m_objects; }

	private:
		explicit Line(const Style& style,
		    LayoutSize size,
		    sap::Length line_spacing,
		    std::vector<std::unique_ptr<LayoutObject>> objs);

	private:
		sap::Length m_line_spacing;
		std::vector<std::unique_ptr<LayoutObject>> m_objects;
	};

	Length calculatePreferredSeparatorWidth(const tree::Separator* sep,
	    bool is_end_of_line,
	    const Style& left_style,
	    const Style& right_style);
}
