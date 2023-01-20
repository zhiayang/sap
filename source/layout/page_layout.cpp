// page_layout.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/page.h" // for Page

#include "layout/base.h" // for RectPageLayout, LayoutObject

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

	void LayoutBase::addObject(std::unique_ptr<LayoutObject> obj)
	{
		m_objects.push_back(std::move(obj));
	}

	PageCursor LayoutBase::newCursor()
	{
		return PageCursor(this, this->new_cursor_payload());
	}

	PageLayout::PageLayout(Size2d size, Length margin) : m_size(size), m_margin(margin)
	{
	}

	PageLayout::~PageLayout()
	{
		m_objects.clear();
	}

	// Size2d PageLayout::size() const
	// {
	// 	return Size2d(m_size.x() - 2 * m_margin, m_size.y() - 2 * m_margin);
	// }

	PageCursor PageLayout::newCursor() const
	{
		return PageCursor(const_cast<PageLayout*>(this), this->new_cursor_payload());
	}

	BasePayload PageLayout::new_cursor_payload() const
	{
		return to_base_payload({
		    .page_num = 0,
		    .pos_on_page = { m_margin, m_margin },
		});
	}

	void PageLayout::delete_cursor_payload(Payload& payload) const
	{
		get_cursor_state(payload).~CursorState();
	}

	BasePayload PageLayout::copy_cursor_payload(const BasePayload& payload) const
	{
		return to_base_payload(get_cursor_state(payload));
	}

	Length PageLayout::get_width_at_cursor_payload(const BasePayload& curs) const
	{
		return m_size.x() - m_margin - get_cursor_state(curs).pos_on_page.x();
	}

	BasePayload PageLayout::move_right(const BasePayload& payload, Length shift) const
	{
		auto& cst = get_cursor_state(payload);
		return to_base_payload({
		    .page_num = cst.page_num,
		    .pos_on_page = cst.pos_on_page + RelativePos::Pos(shift, 0),
		});
	}

	BasePayload PageLayout::new_line(const BasePayload& payload, Length line_height, bool* made_new_page)
	{
		auto& cst = get_cursor_state(payload);
		if(cst.pos_on_page.y() + line_height >= m_size.y() - m_margin)
		{
			m_num_pages = std::max(m_num_pages, cst.page_num + 1 + 1);

			*made_new_page = true;
			return to_base_payload({
			    .page_num = cst.page_num + 1,
			    .pos_on_page = RelativePos::Pos(m_margin, m_margin),
			});
		}
		else
		{
			*made_new_page = false;
			return to_base_payload({
			    .page_num = cst.page_num,
			    .pos_on_page = RelativePos::Pos(m_margin, cst.pos_on_page.y() + line_height),
			});
		}
	}

	RelativePos PageLayout::get_position_on_page(const Payload& payload) const
	{
		auto& cst = get_cursor_state(payload);
		return RelativePos {
			.pos = cst.pos_on_page,
			.page_num = cst.page_num,
		};
	}

	AbsolutePagePos PageLayout::convertPosition(RelativePos pos) const
	{
		// PageLayout doesn't do anything with the size.
		return AbsolutePagePos {
			.pos = Position(pos.pos.x(), pos.pos.y()),
			.page_num = pos.page_num,
		};
	}

	std::vector<pdf::Page*> PageLayout::render() const
	{
		std::vector<pdf::Page*> ret;
		for(size_t i = 0; i < m_num_pages; ++i)
		{
			// TODO: Pages should have the size determined by page size
			ret.emplace_back(util::make<pdf::Page>());
		}

		for(auto& obj : m_objects)
			obj->render(this, ret);

		return ret;
	}
}
