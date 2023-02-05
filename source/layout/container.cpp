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

	void Container::render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		for(auto& obj : m_objects)
			obj->render(layout, pages);
	}
}
