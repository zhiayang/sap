// container.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <span>
#include "layout/layout_object.h"

namespace sap::layout
{
	struct LayoutBase;

	struct Container : LayoutObject
	{
		enum class Direction
		{
			None,
			Vertical,
			Horizontal,
		};

		explicit Container(const Style* style,
			LayoutSize size,
			Direction direction,
			Length top_to_baseline_dist,
			std::vector<std::unique_ptr<LayoutObject>> objs);

		void addObject(std::unique_ptr<LayoutObject> obj);
		std::vector<std::unique_ptr<LayoutObject>>& objects();
		const std::vector<std::unique_ptr<LayoutObject>>& objects() const;

		virtual bool requires_space_reservation() const override;
		virtual layout::PageCursor compute_position_impl(layout::PageCursor cursor) override;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		Direction m_direction;
		Length m_tallest_ascent;
		std::vector<std::unique_ptr<LayoutObject>> m_objects;
	};

	template <typename T>
	layout::PageCursor position_children_in_container(layout::PageCursor cursor,
		Length self_width,
		Container::Direction direction,
		Alignment horz_alignment,
		Length top_to_baseline_dist,
		Length vert_obj_spacing,
		bool shift_by_ascent_of_first_child,
		const std::vector<std::unique_ptr<T>>& objects);
}
