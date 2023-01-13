// centred_layout.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/base.h"

namespace sap::layout
{
	using BasePayload = LayoutBase::Payload;
	struct CursorState
	{
		size_t page_num;
		RelativePos::Pos pos_on_page;
	};

	static_assert(sizeof(CursorState) <= sizeof(BasePayload));

	static const CursorState& get_cursor_state(const BasePayload& payload)
	{
		return *reinterpret_cast<const CursorState*>(&payload.payload[0]);
	}

	static BasePayload to_base_payload(CursorState payload)
	{
		BasePayload ret {};
		new(&ret.payload[0]) CursorState(std::move(payload));

		return ret;
	}


	CentredLayout::CentredLayout(LayoutBase* parent, LineCursor parent_cursor) : m_parent(parent), m_parent_cursor(parent_cursor)
	{
		m_layout_position = parent_cursor.position();
	}

	CentredLayout::~CentredLayout()
	{
		m_objects.clear();
	}

	Size2d CentredLayout::size() const
	{
		return m_parent->size();
	}

	BasePayload CentredLayout::new_cursor_payload() const
	{
		return to_base_payload({
		    .page_num = 0,
		    .pos_on_page = { 0, 0 },
		});
	}

	void CentredLayout::delete_cursor_payload(Payload& payload) const
	{
		get_cursor_state(payload).~CursorState();
	}

	BasePayload CentredLayout::copy_cursor_payload(const Payload& payload) const
	{
		return to_base_payload(get_cursor_state(payload));
	}

	RelativePos CentredLayout::get_position_on_page(const Payload& payload) const
	{
		auto& cst = get_cursor_state(payload);
		return RelativePos {
			.pos = cst.pos_on_page,
			.page_num = cst.page_num,
		};
	}

	BasePayload CentredLayout::new_line(const Payload& payload, Length line_height, bool* made_new_page)
	{
		auto& cst = get_cursor_state(payload);

		m_parent_cursor = m_parent_cursor.newLine(line_height, made_new_page);
		if(*made_new_page)
		{
			return to_base_payload({
			    .page_num = cst.page_num + 1,
			    .pos_on_page = RelativePos::Pos(0, 0),
			});
		}
		else
		{
			return to_base_payload({
			    .page_num = cst.page_num,
			    .pos_on_page = RelativePos::Pos(0, cst.pos_on_page.y() + line_height),
			});
		}
	}

	Length CentredLayout::get_width_at_cursor_payload(const Payload& payload) const
	{
		return m_parent_cursor.widthAtCursor();
	}

	BasePayload CentredLayout::move_right(const BasePayload& payload, Length shift) const
	{
		// zpr::println("line width = {}", m_current_line_width);

		auto& cst = get_cursor_state(payload);
		return to_base_payload({
		    .page_num = cst.page_num,
		    .pos_on_page = cst.pos_on_page + RelativePos::Pos(shift, 0),
		});
	}

	void CentredLayout::computeLayoutSize()
	{
		// TODO: inter-object margin!
		for(auto& obj : m_objects)
		{
			m_layout_size.x() = std::max(m_layout_size.x(), obj->layoutSize().x());
			m_layout_size.y() += obj->layoutSize().y();
		}
	}

	AbsolutePagePos CentredLayout::convertPosition(RelativePos pos) const
	{
		auto pos_in_parent = m_parent->convertPosition(m_layout_position);

		auto x = (m_parent->size().x() - m_layout_size.x()) / 2;

		return AbsolutePagePos {
			.pos = pos_in_parent.pos + Position(x + pos.pos.x(), pos.pos.y()),
			.page_num = pos_in_parent.page_num + pos.page_num,
		};
	}

	LineCursor CentredLayout::parentCursor() const
	{
		return m_parent_cursor;
	}

	void CentredLayout::render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		for(auto& obj : m_objects)
			obj->render(this, pages);
	}
}
