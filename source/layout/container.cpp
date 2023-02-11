// container.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/container.h"

namespace sap::layout
{
	Container::Container(const Style* style, //
		LayoutSize size,
		Direction direction,
		Length top_to_baseline,
		std::vector<std::unique_ptr<LayoutObject>> objs)
		: LayoutObject(style, std::move(size))
		, m_direction(direction)
		, m_top_to_baseline_dist(top_to_baseline)
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
		using enum Alignment;
		using enum Direction;

		this->positionRelatively(cursor.position());

		if(m_objects.empty())
			return cursor;

		// for containers, ascent is always 0 -- the top-left corner is always the origin
		assert(m_layout_size.ascent == 0);
		cursor = cursor.newLine(m_top_to_baseline_dist);

		Length obj_spacing = 0;
		switch(m_direction)
		{
			case Vertical: {
				// TODO: support vertical alignment for vbox.
				obj_spacing = m_style->paragraph_spacing();
				break;
			}

			case None: {
				obj_spacing = 0;
				break;
			}

			case Horizontal: {
				// TODO: specify inter-object spacing properly, right now there's 0.
				auto total_obj_width = std::accumulate(m_objects.begin(), m_objects.end(), Length(0),
					[](auto a, const auto& b) {
						return a + b->layoutSize().width;
					});

				auto self_width = m_layout_size.width;
				auto space_width = std::max(Length(0), self_width - total_obj_width);
				auto horz_space = cursor.widthAtCursor();

				switch(m_style->alignment())
				{
					case Left: {
						obj_spacing = 0;
						break;
					}

					case Right: {
						obj_spacing = 0;
						cursor = cursor.moveRight(horz_space);
						cursor = cursor.moveRight(space_width);
						break;
					}

					case Centre: {
						obj_spacing = 0;
						cursor = cursor.moveRight((horz_space - self_width) / 2);
						cursor = cursor.moveRight(space_width / 2);
						break;
					}

					case Justified: {
						assert(not m_objects.empty());
						if(m_objects.size() == 1)
							obj_spacing = 0;
						else
							obj_spacing = space_width / static_cast<double>(m_objects.size() - 1);

						break;
					}
				}
				break;
			}
		}

		bool is_first_child = true;
		for(auto& child : m_objects)
		{
			// if we are vertically stacked, we need to move the cursor horizontally to
			// preserve horizontal alignment.
			if(m_direction == Vertical || m_direction == None)
			{
				cursor = cursor.carriageReturn();

				auto horz_space = cursor.widthAtCursor();
				auto self_width = m_layout_size.width;

				auto space_width = std::max(Length(0), self_width - child->layoutSize().width);
				switch(m_style->alignment())
				{
					case Left:
					case Justified: break;

					case Right: {
						cursor = cursor.moveRight(horz_space - self_width);
						cursor = cursor.moveRight(space_width);
						break;
					}

					case Centre: {
						cursor = cursor.moveRight((horz_space - self_width) / 2);
						cursor = cursor.moveRight(space_width / 2);
						break;
					}
				}
			}

			switch(m_direction)
			{
				case None: {
					child->positionChildren(cursor);
					break;
				}

				case Horizontal: {
					child->positionChildren(cursor.limitWidth(child->layoutSize().width));
					cursor = cursor.moveRight(child->layoutSize().width + obj_spacing);
					break;
				}

				case Vertical: {
					// auto& tmp = *child;
					// zpr::println("child({})@{}: a={}, d={}", typeid(tmp).name(), cursor.position().page_num,
					// 	child->layoutSize().ascent, child->layoutSize().descent);

					if(not is_first_child)
						cursor = cursor.newLine(obj_spacing);

					cursor = child->positionChildren(cursor.newLine(child->layoutSize().ascent));

					is_first_child = false;
					break;
				}
			}
		}

		switch(m_direction)
		{
			case None:
			case Horizontal: //
				return cursor.newLine(m_layout_size.total_height() - m_top_to_baseline_dist);

			case Vertical: //
				return cursor;
		}
	}

	void Container::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		for(auto& obj : m_objects)
			obj->render(layout, pages);
	}
}
