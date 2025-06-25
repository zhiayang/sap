// container.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/path.h"

#include "layout/path.h"
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

		explicit Container(const Style& style,
		    LayoutSize size,
		    Direction direction,
		    bool glue_objects,
		    BorderStyle border_style,
		    std::vector<std::unique_ptr<LayoutObject>> objs,
		    std::optional<Length> override_obj_spacing = std::nullopt);

		void addObject(std::unique_ptr<LayoutObject> obj);
		std::vector<std::unique_ptr<LayoutObject>>& objects();
		const std::vector<std::unique_ptr<LayoutObject>>& objects() const;

		virtual bool requires_space_reservation() const override;
		virtual layout::PageCursor compute_position_impl(layout::PageCursor cursor) override;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		bool m_glued;
		Direction m_direction;
		BorderStyle m_border_style;
		std::vector<std::unique_ptr<LayoutObject>> m_objects;
		std::optional<Length> m_override_obj_spacing {};

		std::vector<layout::Path> m_border_objects {};
	};
}
