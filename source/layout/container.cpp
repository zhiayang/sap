// container.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/container.h"

namespace sap::layout
{
	Container::Container(Size2d size) : LayoutObject(std::move(size))
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
		cursor = cursor.newLine(0).carriageReturn();
		this->positionRelatively(cursor.position());

		for(auto& child : m_objects)
		{
			zpr::println("# PC {}", (void*) child.get());
			cursor = child->positionChildren(cursor);
		}

		return cursor;
	}

	void Container::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		for(auto& obj : m_objects)
		{
			zpr::println("# RC {}", (void*) obj.get());
			obj->render(layout, pages);
		}
	}
}
