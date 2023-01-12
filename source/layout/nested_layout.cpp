// nested_layout.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/base.h"

namespace sap::layout
{
	using BasePayload = LayoutBase::Payload;

	NestedLayout::NestedLayout(LayoutBase* parent) : m_parent(parent)
	{
	}

	Size2d NestedLayout::size() const
	{
		return m_parent->size();
	}

	BasePayload NestedLayout::new_cursor_payload() const
	{
		return m_parent->new_cursor_payload();
	}

	void NestedLayout::delete_cursor_payload(Payload& payload) const
	{
		m_parent->delete_cursor_payload(payload);
	}

	BasePayload NestedLayout::copy_cursor_payload(const BasePayload& payload) const
	{
		return m_parent->copy_cursor_payload(payload);
	}

	Length NestedLayout::get_width_at_cursor_payload(const BasePayload& payload) const
	{
		return m_parent->get_width_at_cursor_payload(payload);
	}

	BasePayload NestedLayout::move_right(const BasePayload& payload, Length shift) const
	{
		return m_parent->move_right(payload, shift);
	}

	BasePayload NestedLayout::new_line(const BasePayload& payload, Length line_height)
	{
		return m_parent->new_line(payload, line_height);
	}

	PagePosition NestedLayout::get_position_on_page(const Payload& payload) const
	{
		return m_parent->get_position_on_page(payload);
	}
}
