// cursor.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/base.h"

namespace sap::layout
{
	LineCursor::~LineCursor()
	{
		m_layout->delete_cursor_payload(m_payload);
	}

	LineCursor::LineCursor(const LineCursor& other)
	    : m_layout(other.m_layout)
	    , m_payload(m_layout->copy_cursor_payload(other.m_payload))
	{
	}

	LineCursor::LineCursor(LayoutBase* layout, LayoutBase::Payload payload) : m_layout(layout), m_payload(std::move(payload))
	{
	}

	LineCursor::LineCursor(LineCursor&& other) : m_layout(other.m_layout), m_payload(std::move(other.m_payload))
	{
	}

	LineCursor& LineCursor::operator=(const LineCursor& other)
	{
		LineCursor copy { other };
		std::swap(m_payload, copy.m_payload);

		return *this;
	}

	LineCursor& LineCursor::operator=(LineCursor&& other)
	{
		LineCursor copy { std::move(other) };
		std::swap(m_payload, copy.m_payload);

		return *this;
	}

	RelativePos LineCursor::position() const
	{
		return m_layout->get_position_on_page(m_payload);
	}

	Length LineCursor::widthAtCursor() const
	{
		return m_layout->get_width_at_cursor_payload(m_payload);
	}

	LineCursor LineCursor::newLine(Length line_height, bool* made_new_page2) const
	{
		bool made_new_page = false;
		auto payload = m_layout->new_line(m_payload, line_height, &made_new_page);

		if(made_new_page2)
			*made_new_page2 = made_new_page;

		return LineCursor(m_layout, std::move(payload));
	}

	LineCursor LineCursor::moveRight(Length shift) const
	{
		return LineCursor(m_layout, m_layout->move_right(m_payload, shift));
	}
}
