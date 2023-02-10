// container.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

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
			Size2d size,
			Direction direction,
			std::vector<std::unique_ptr<LayoutObject>> objs);

		void addObject(std::unique_ptr<LayoutObject> obj);
		std::vector<std::unique_ptr<LayoutObject>>& objects();
		const std::vector<std::unique_ptr<LayoutObject>>& objects() const;

		virtual layout::PageCursor positionChildren(layout::PageCursor cursor) override;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		Direction m_direction;
		std::vector<std::unique_ptr<LayoutObject>> m_objects;
	};
}
