// container.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/container.h"

namespace sap::layout
{
	Container::Container(const Style* style, Size2d size, Direction direction, std::vector<std::unique_ptr<LayoutObject>> objs)
		: LayoutObject(style, std::move(size))
		, m_direction(direction)
		, m_objects(std::move(objs))
	{
	}

	void Container::addObject(std::unique_ptr<LayoutObject> obj)
	{
		m_objects.push_back(std::move(obj));
	}

	std::vector<std::unique_ptr<LayoutObject>>& Container::objects()
	{
		return m_objects;
	}

	const std::vector<std::unique_ptr<LayoutObject>>& Container::objects() const
	{
		return m_objects;
	}

	layout::PageCursor Container::positionChildren(layout::PageCursor cursor)
	{
		this->positionRelatively(cursor.position());

		// TODO: support vertical alignment for vbox.
		Length obj_spacing = 0;
		switch(m_direction)
		{
			case Direction::Vertical: {
				obj_spacing = m_style->paragraph_spacing();
				break;
			}

			case Direction::Horizontal: {
				auto total_width = m_layout_size.x();
				auto obj_width = std::accumulate(m_objects.begin(), m_objects.end(), Length(0), [](auto a, const auto& b) {
					return a + b->layoutSize().x();
				});

				obj_spacing = (total_width - obj_width) / static_cast<double>(m_objects.size() - 1);
				break;
			}
		}



		for(auto& child : m_objects)
		{
			cursor = child->positionChildren(cursor);

			switch(m_direction)
			{
				using enum Direction;
				case Horizontal: //
					cursor = cursor.moveRight(obj_spacing);
					break;

				case Vertical: //
					cursor = cursor.newLine(obj_spacing);
					break;
			}
		}

		return cursor;
	}

	void Container::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		for(auto& obj : m_objects)
			obj->render(layout, pages);
	}
}
