// cursor.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/base.h"

namespace sap::layout
{
	PageCursor::~PageCursor()
	{
		m_layout->delete_cursor_payload(m_payload);
	}

	PageCursor::PageCursor(const PageCursor& other)
	    : m_layout(other.m_layout)
	    , m_payload(m_layout->copy_cursor_payload(other.m_payload))
	{
	}

	PageCursor::PageCursor(LayoutBase* layout, LayoutBase::Payload payload) : m_layout(layout), m_payload(std::move(payload))
	{
	}

	PageCursor::PageCursor(PageCursor&& other) : m_layout(other.m_layout), m_payload(std::move(other.m_payload))
	{
	}

	PageCursor& PageCursor::operator=(const PageCursor& other)
	{
		PageCursor copy { other };
		std::swap(m_payload, copy.m_payload);

		return *this;
	}

	PageCursor& PageCursor::operator=(PageCursor&& other)
	{
		PageCursor copy { std::move(other) };
		std::swap(m_payload, copy.m_payload);

		return *this;
	}

	RelativePos PageCursor::position() const
	{
		return m_layout->get_position_on_page(m_payload);
	}

	Length PageCursor::widthAtCursor() const
	{
		return m_layout->get_width_at_cursor_payload(m_payload);
	}

	PageCursor PageCursor::newLine(Length line_height, bool* made_new_page2) const
	{
		bool made_new_page = false;
		auto payload = m_layout->new_line(m_payload, line_height, &made_new_page);

		if(made_new_page2)
			*made_new_page2 = made_new_page;

		return PageCursor(m_layout, std::move(payload));
	}

	PageCursor PageCursor::ensureVerticalSpace(Length vert, bool* made_new_page2) const
	{
		bool made_new_page = false;
		auto payload = m_layout->new_line(m_payload, vert, &made_new_page);

		// if we didn't have to make a new page, it means we had enough space;
		// so don't return the new cursor, and continue layout on the current page.
		if(not made_new_page)
			return *this;

		if(made_new_page2)
			*made_new_page2 = true;
		return PageCursor(m_layout, std::move(payload));
	}

	PageCursor PageCursor::moveRight(Length shift) const
	{
		return PageCursor(m_layout, m_layout->move_right(m_payload, shift));
	}

	PageCursor PageCursor::carriageReturn() const
	{
		return PageCursor(m_layout, m_layout->carriage_return(m_payload));
	}

	PageCursor PageCursor::moveToPosition(RelativePos pos) const
	{
		return PageCursor(m_layout, m_layout->move_to_position(m_payload, pos));
	}
}
