// base.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/style.h" // for Stylable
#include "sap/units.h" // for Length, Vector2

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
	struct LayoutBase;


	struct RelativePos
	{
		struct TAG_RELATIVE;
		using Pos = dim::Vector2<sap::Length::unit_system, TAG_RELATIVE>;
		Pos pos;
		size_t page_num;
	};

	struct AbsolutePagePos
	{
		Position pos;
		size_t page_num;
	};

	/*
	    As mentioned in the overview above, a LayoutObject is some object that can be laid out and
	    rendered into the document.
	*/
	struct LayoutObject : Stylable
	{
		LayoutObject(RelativePos position, Size2d size) : m_layout_position(std::move(position)), m_layout_size(size) { }
		virtual ~LayoutObject() = default;

		LayoutObject& operator=(LayoutObject&&) = default;
		LayoutObject(LayoutObject&&) = default;

		Size2d layoutSize() const { return m_layout_size; }
		RelativePos layoutPosition() const { return m_layout_position; }

		/*
		    Render (emit PDF commands) the object. Must be called after layout(). For now, we render directly to
		    the PDF page (by construcitng and emitting PageObjects), instead of returning a pageobject -- some
		    layout objects might require multiple pdf page objects, so this is a more flexible design.
		*/
		virtual void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const = 0;

	protected:
		RelativePos m_layout_position;
		Size2d m_layout_size;
	};


	struct PageCursor;

	struct LayoutBase
	{
		struct Payload
		{
			uint8_t payload[32];
		};

		friend struct PageCursor;

		virtual ~LayoutBase() = default;

		PageCursor newCursor();
		void addObject(std::unique_ptr<LayoutObject> obj);

		// virtual Size2d size() const = 0;
		virtual AbsolutePagePos convertPosition(RelativePos pos) const = 0;

		virtual Payload new_cursor_payload() const = 0;
		virtual void delete_cursor_payload(Payload& payload) const = 0;
		virtual Payload copy_cursor_payload(const Payload& payload) const = 0;

		virtual Length get_width_at_cursor_payload(const Payload& payload) const = 0;
		virtual RelativePos get_position_on_page(const Payload& payload) const = 0;

		virtual Payload new_line(const Payload& payload, Length line_height, bool* made_new_page) = 0;
		virtual Payload move_right(const Payload& payload, Length shift) const = 0;

	protected:
		std::vector<std::unique_ptr<LayoutObject>> m_objects {};
	};

	struct PageCursor
	{
		PageCursor(LayoutBase* layout, LayoutBase::Payload payload);
		~PageCursor();

		PageCursor(const PageCursor& other);
		PageCursor& operator=(const PageCursor& other);

		PageCursor(PageCursor&& other);
		PageCursor& operator=(PageCursor&& other);

		Length widthAtCursor() const;
		[[nodiscard]] PageCursor moveRight(Length shift) const;
		[[nodiscard]] PageCursor newLine(Length line_height, bool* made_new_page = nullptr) const;

		RelativePos position() const;
		LayoutBase* layout() const { return m_layout; }

	private:
		LayoutBase* m_layout;
		LayoutBase::Payload m_payload;
	};

	struct PageLayout final : LayoutBase
	{
		explicit PageLayout(Size2d size, Length margin);
		~PageLayout();

		PageCursor newCursor() const;
		std::vector<pdf::Page*> render() const;

		// virtual Size2d size() const override;
		virtual AbsolutePagePos convertPosition(RelativePos pos) const override;

	private:
		virtual Payload new_cursor_payload() const override;
		virtual void delete_cursor_payload(Payload& payload) const override;

		virtual Payload copy_cursor_payload(const Payload& payload) const override;
		virtual RelativePos get_position_on_page(const Payload& payload) const override;

		virtual Payload new_line(const Payload& payload, Length line_height, bool* made_new_page) override;
		virtual Payload move_right(const Payload& payload, Length shift) const override;
		virtual Length get_width_at_cursor_payload(const Payload& payload) const override;

	private:
		Size2d m_size;
		Length m_margin;
		size_t m_num_pages = 1;
	};
}
