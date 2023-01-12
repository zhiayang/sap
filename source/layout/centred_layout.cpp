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
		Vector2 pos_on_page;
	};

	static_assert(sizeof(CursorState) <= sizeof(BasePayload));

	static const CursorState& get_cursor_state(const BasePayload& payload)
	{
		return *reinterpret_cast<const CursorState*>(&payload.payload[0]);
	}

	static CursorState& get_cursor_state(BasePayload& payload)
	{
		return *reinterpret_cast<CursorState*>(&payload.payload[0]);
	}

	static BasePayload to_base_payload(CursorState payload)
	{
		BasePayload ret {};
		new(&ret.payload[0]) CursorState(std::move(payload));

		return ret;
	}


	CentredLayout::CentredLayout(LayoutBase* parent) : NestedLayout(parent), m_content_width()
	{
	}

	BasePayload CentredLayout::move_right(const BasePayload& payload, Length shift) const
	{
		auto& cst = get_cursor_state(payload);
		m_content_width += shift;

		return to_base_payload({
		    .page_num = cst.page_num,
		    .pos_on_page = cst.pos_on_page + Offset2d(shift, 0),
		});
	}

	void CentredLayout::render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		for(auto& obj : m_objects)
			obj->render(this, pages);
	}

	CentredLayout::~CentredLayout()
	{
	}
}
