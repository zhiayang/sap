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
	using Size2d = Vector2;
	using Position = Vector2;
	using Offset2d = Vector2;

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

	/*
	    As mentioned in the overview above, a LayoutObject is some object that can be laid out and
	    rendered into the document.
	*/
	struct LayoutObject : Stylable
	{
		LayoutObject() = default;
		LayoutObject& operator=(LayoutObject&&) = default;
		LayoutObject(LayoutObject&&) = default;
		virtual ~LayoutObject() = default;

		/*
		    Render (emit PDF commands) the object. Must be called after layout(). For now, we render directly to
		    the PDF page (by construcitng and emitting PageObjects), instead of returning a pageobject -- some
		    layout objects might require multiple pdf page objects, so this is a more flexible design.
		*/
		virtual void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const = 0;
	};

	struct RelativePos
	{
		struct TAG_RELATIVE;
		using Pos = dim::Vector2<sap::Length::unit_system, TAG_RELATIVE>;
		Pos pos;
		size_t page_num;
	};

	struct PagePosition
	{
		Position pos;
		size_t page_num;
	};


	struct LineCursor;

	struct LayoutBase
	{
		struct Payload
		{
			uint8_t payload[32];
		};

		friend struct LineCursor;

		void addObject(std::unique_ptr<LayoutObject> obj);

		virtual Size2d size() const = 0;
		virtual PagePosition convertPosition(RelativePos pos) const = 0;

		virtual Payload new_cursor_payload() const = 0;
		virtual void delete_cursor_payload(Payload& payload) const = 0;
		virtual Payload copy_cursor_payload(const Payload& payload) const = 0;

		virtual Length get_width_at_cursor_payload(const Payload& payload) const = 0;
		virtual RelativePos get_position_on_page(const Payload& payload) const = 0;

		virtual Payload new_line(const Payload& payload, Length line_height) = 0;
		virtual Payload move_right(const Payload& payload, Length shift) const = 0;

		std::vector<std::unique_ptr<LayoutObject>> m_objects {};
	};

	struct LineCursor
	{
		LineCursor(LayoutBase* layout, LayoutBase::Payload payload);
		~LineCursor();

		LineCursor(const LineCursor& other);
		LineCursor& operator=(const LineCursor& other);

		LineCursor(LineCursor&& other);
		LineCursor& operator=(LineCursor&& other);

		Length widthAtCursor() const;
		LineCursor moveRight(Length shift) const;
		LineCursor newLine(Length line_height) const;

		RelativePos position() const;

	private:
		LayoutBase* m_layout;
		LayoutBase::Payload m_payload;
	};




	struct PageLayout : LayoutBase
	{
		explicit PageLayout(Size2d size, Length margin);

		LineCursor newCursor() const;
		std::vector<pdf::Page*> render() const;

		virtual Size2d size() const override;
		virtual PagePosition convertPosition(RelativePos pos) const override;

	private:
		virtual Payload new_cursor_payload() const override;
		virtual void delete_cursor_payload(Payload& payload) const override;

		virtual Payload copy_cursor_payload(const Payload& payload) const override;
		virtual RelativePos get_position_on_page(const Payload& payload) const override;

		virtual Payload new_line(const Payload& payload, Length line_height) override;
		virtual Payload move_right(const Payload& payload, Length shift) const override;
		virtual Length get_width_at_cursor_payload(const Payload& payload) const override;

	private:
		Size2d m_size;
		Length m_margin;
		size_t m_num_pages = 1;
	};



	struct NestedLayout : LayoutBase, LayoutObject
	{
		virtual Size2d size() const override;

	protected:
		explicit NestedLayout(LayoutBase* parent);

		virtual Payload new_cursor_payload() const override;
		virtual void delete_cursor_payload(Payload& payload) const override;

		virtual Payload copy_cursor_payload(const Payload& payload) const override;
		virtual RelativePos get_position_on_page(const Payload& payload) const override;

		virtual Payload new_line(const Payload& payload, Length line_height) override;
		virtual Payload move_right(const Payload& payload, Length shift) const override;
		virtual Length get_width_at_cursor_payload(const Payload& payload) const override;

		LayoutBase* m_parent;
	};


	struct CentredLayout : NestedLayout
	{
		explicit CentredLayout(LayoutBase* parent);
		~CentredLayout();

		virtual PagePosition convertPosition(RelativePos pos) const override;
		virtual void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		virtual Payload move_right(const Payload& payload, Length shift) const override;

	private:
		mutable Length m_content_width;
	};
}
