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
		bool is_absolute;
		Length limited_width = Length(INFINITY);
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

	LayoutObject* LayoutBase::addObject(std::unique_ptr<LayoutObject> obj)
	{
		return m_objects.emplace_back(std::move(obj)).get();
	}

	PageCursor LayoutBase::newCursor()
	{
		return PageCursor(this, this->new_cursor_payload());
	}





	PageLayout::PageLayout(Size2d size, PageLayout::Margins margins) : m_size(size), m_margins(std::move(margins))
	{
		m_content_size = {
			size.x() - m_margins.left - m_margins.right,
			size.y() - m_margins.top - m_margins.bottom,
		};
	}

	PageLayout::~PageLayout()
	{
		m_objects.clear();
	}

	size_t PageLayout::pageCount() const
	{
		return m_num_pages;
	}

	Size2d PageLayout::pageSize() const
	{
		return m_size;
	}

	Size2d PageLayout::contentSize() const
	{
		return m_content_size;
	}

	PageCursor PageLayout::newCursor() const
	{
		return PageCursor(const_cast<PageLayout*>(this), this->new_cursor_payload());
	}

	PageCursor PageLayout::newCursorAtPosition(AbsolutePagePos pos) const
	{
		return PageCursor(const_cast<PageLayout*>(this),
			to_base_payload({
				.page_num = pos.page_num,
				.pos_on_page = { pos.pos.x(), pos.pos.y() },
				.is_absolute = true,
			}));
	}

	BasePayload PageLayout::new_cursor_payload() const
	{
		return to_base_payload({
			.page_num = 0,
			.pos_on_page = { 0, 0 },
			.is_absolute = false,
		});
	}

	BasePayload PageLayout::move_to_position(const Payload& payload, RelativePos pos) const
	{
		return to_base_payload({
			.page_num = pos.page_num,
			.pos_on_page = pos.pos,
			.is_absolute = false,
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
		return std::min(                                                //
			get_cursor_state(curs).limited_width,                       //
			m_content_size.x() - get_cursor_state(curs).pos_on_page.x() //
		);
	}

	BasePayload PageLayout::limit_width(const BasePayload& payload, Length width) const
	{
		auto cst = get_cursor_state(payload);
		cst.limited_width = std::min(width, this->get_width_at_cursor_payload(payload));

		return to_base_payload(cst);
	}

	BasePayload PageLayout::move_right(const BasePayload& payload, Length shift) const
	{
		auto& cst = get_cursor_state(payload);
		return to_base_payload({
			.page_num = cst.page_num,
			.pos_on_page = cst.pos_on_page + RelativePos::Pos(shift, 0),
			.is_absolute = cst.is_absolute,
		});
	}

	BasePayload PageLayout::carriage_return(const BasePayload& payload) const
	{
		auto& cst = get_cursor_state(payload);
		return to_base_payload({
			.page_num = cst.page_num,
			.pos_on_page = { 0, cst.pos_on_page.y() },
			.is_absolute = cst.is_absolute,
		});
	}

	BasePayload PageLayout::new_line(const BasePayload& payload, Length line_height, bool* made_new_page)
	{
		auto& cst = get_cursor_state(payload);
		if(not cst.is_absolute && cst.pos_on_page.y() + line_height >= m_content_size.y())
		{
			m_num_pages = std::max(m_num_pages, cst.page_num + 1 + 1);

			*made_new_page = true;
			return to_base_payload({
				.page_num = cst.page_num + 1,
				.pos_on_page = { cst.pos_on_page.x(), 0 },
				.is_absolute = cst.is_absolute,
			});
		}
		else
		{
			*made_new_page = false;
			return to_base_payload({
				.page_num = cst.page_num,
				.pos_on_page = RelativePos::Pos(cst.pos_on_page.x(), cst.pos_on_page.y() + line_height),
				.is_absolute = cst.is_absolute,
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
			.pos = Position(m_margins.left + pos.pos.x(), m_margins.top + pos.pos.y()),
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
