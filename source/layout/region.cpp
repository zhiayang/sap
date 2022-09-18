// region.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"

namespace sap::layout
{
	LayoutRegion::LayoutRegion(Size2d size) : m_size(std::move(size))
	{
	}

	void LayoutRegion::render(interp::Interpreter* cs, Position position, pdf::Page* page) const
	{
		for(auto& [pos, obj] : m_objects)
			obj->render(cs, this, pos + position, page);
	}

	Position LayoutRegion::cursor() const
	{
		return m_cursor;
	}

	void LayoutRegion::moveCursorTo(Position pos)
	{
		if(pos.x() > m_size.x() || pos.y() > m_size.y())
			sap::warn("layout", "new cursor position '{}' outside of region bounds '{}'", pos, m_size);

		m_cursor = pos;
	}

	void LayoutRegion::advanceCursorBy(Offset2d offset)
	{
		this->moveCursorTo(m_cursor + offset);
	}

	void LayoutRegion::addObjectAtCursor(LayoutObject* obj)
	{
		m_objects.emplace_back(m_cursor, obj);
	}

	void LayoutRegion::addObjectAtPosition(LayoutObject* obj, Position pos)
	{
		if(pos.x() > m_size.x() || pos.y() > m_size.y())
			sap::warn("layout", "position '{}' for object outside of region bounds '{}'", pos, m_size);
	}

	Size2d LayoutRegion::spaceAtCursor() const
	{
		return (m_size - m_cursor).into(Size2d {});
	}

	bool LayoutRegion::haveSpaceAtCursor(Size2d size) const
	{
		auto space = spaceAtCursor();
		return space.x() >= size.x() && space.y() >= size.y();
	}
}
