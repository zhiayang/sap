// base.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/style.h"
#include "sap/units.h"

#include "layout/cursor.h"
#include "layout/layout_object.h"

namespace pdf
{
	struct Page;
}

namespace sap
{
	namespace tree
	{
		struct Text;
		struct Paragraph;
		struct Document;
	}

	namespace interp
	{
		struct Interpreter;
	}
}

namespace sap::layout
{
	struct Document;
	struct LayoutBase;

	struct PageCursor;

	struct LayoutBase
	{
		using Payload = CursorPayload;
		friend struct PageCursor;

		virtual ~LayoutBase() = default;

		PageCursor newCursor();
		LayoutObject* addObject(std::unique_ptr<LayoutObject> obj);

		// virtual Size2d size() const = 0;
		virtual AbsolutePagePos convertPosition(RelativePos pos) const = 0;

		virtual Payload new_cursor_payload() const = 0;
		virtual void delete_cursor_payload(Payload& payload) const = 0;
		virtual Payload copy_cursor_payload(const Payload& payload) const = 0;

		virtual Length get_width_at_cursor_payload(const Payload& payload) const = 0;
		virtual Length get_vertical_space_at_cursor_payload(const Payload& payload) const = 0;
		virtual RelativePos get_position_on_page(const Payload& payload) const = 0;

		virtual Payload new_line(const Payload& payload, Length line_height, bool* made_new_page) = 0;
		virtual Payload move_right(const Payload& payload, Length shift) const = 0;
		virtual Payload move_down(const Payload& payload, Length shift) = 0;
		virtual Payload move_to_position(const Payload& payload, RelativePos pos) const = 0;
		virtual Payload carriage_return(const Payload& payload) const = 0;
		virtual Payload limit_width(const Payload& payload, Length width) const = 0;
		virtual Payload new_page(const Payload& payload) = 0;

		Document* document() const { return m_document; }

	protected:
		LayoutBase(Document* doc) : m_document(doc) { }

	protected:
		Document* m_document;
		std::vector<std::unique_ptr<LayoutObject>> m_objects {};
	};

	struct PageLayout final : LayoutBase
	{
		struct Margins
		{
			Length top;
			Length bottom;
			Length left;
			Length right;
		};

		explicit PageLayout(Document* doc, Size2d size, PageLayout::Margins margins);
		~PageLayout();

		PageCursor newCursor() const;
		PageCursor newCursorAtPosition(AbsolutePagePos pos) const;
		std::vector<pdf::Page*> render() const;

		size_t pageCount() const;
		Size2d pageSize() const;
		Size2d contentSize() const;

		virtual AbsolutePagePos convertPosition(RelativePos pos) const override;

	private:
		virtual Payload new_cursor_payload() const override;
		virtual void delete_cursor_payload(Payload& payload) const override;

		virtual Payload copy_cursor_payload(const Payload& payload) const override;
		virtual RelativePos get_position_on_page(const Payload& payload) const override;

		virtual Payload new_line(const Payload& payload, Length line_height, bool* made_new_page) override;
		virtual Payload move_right(const Payload& payload, Length shift) const override;
		virtual Payload move_down(const Payload& payload, Length shift) override;
		virtual Length get_width_at_cursor_payload(const Payload& payload) const override;
		virtual Length get_vertical_space_at_cursor_payload(const Payload& payload) const override;
		virtual Payload move_to_position(const Payload& payload, RelativePos pos) const override;
		virtual Payload carriage_return(const Payload& payload) const override;
		virtual Payload limit_width(const Payload& payload, Length width) const override;
		virtual Payload new_page(const Payload& payload) override;

	private:
		Size2d m_size;
		Margins m_margins;

		size_t m_num_pages = 1;
		Size2d m_content_size;
	};
}
