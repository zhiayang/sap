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

#if 0
	/*
	    A layout region holds several objects, where each object is placed at a certain position. During the
	    layout process, the region has a cursor, which can be used as a convenient method to place objects
	    that "flow" across a page (each new object then advances the cursor by however large it is).

	    Positions are relative to the origin of the region, which is the top-left corner of its bounding box.

	    A region is used as a "target" inside which objects are placed during the layout phase. In the event
	    of nested layout areas (eg. a two-column layout), the LayoutObject (in this case, the two-column) would
	    be responsible for managing separate regions, such that a layout region is not a recursive structure.

	    For the most basic case, one page contains one layout region.

	    During the rendering phase, the layout region simply calls the render method on the objects it contains,
	    sequentially. Since layout has been completed, objects are expected to render directly to the PDF page.
	*/
	struct Cursor
	{
		size_t page_num;
		Position pos_on_page;
	};

	struct RectPageLayout
	{
		explicit RectPageLayout(Size2d size, Length margin) : m_size(size), m_margin(margin) { }

		Cursor newCursor() const { return { SIZE_MAX, m_size }; }
		Length getWidthAt(Cursor curs) const
		{
			return curs.page_num == SIZE_MAX ? 0 : m_size.x() - m_margin - curs.pos_on_page.x();
		}
		Cursor newLineFrom(Cursor curs, Length line_height)
		{
			if(curs.pos_on_page.y() + line_height >= m_size.y() - m_margin)
			{
				m_num_pages = std::max(m_num_pages, curs.page_num + 1 + 1);
				return { curs.page_num + 1, Position(m_margin, m_margin + line_height) };
			}
			else
			{
				return { curs.page_num, Position(m_margin, curs.pos_on_page.y() + line_height) };
			}
		}

		Cursor moveRightFrom(Cursor curs, Length width) const { return { curs.page_num, curs.pos_on_page + Offset2d(width, 0) }; }

		std::vector<pdf::Page*> render() const;

		void addObject(std::unique_ptr<LayoutObject> obj) { m_objects.push_back(std::move(obj)); }

	private:
		Size2d m_size;
		Length m_margin;
		size_t m_num_pages = 1;
		std::vector<std::unique_ptr<LayoutObject>> m_objects {};
	};
#endif

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

	protected:
		virtual Payload new_cursor_payload() const = 0;
		virtual void delete_cursor_payload(Payload& payload) const = 0;
		virtual Payload copy_cursor_payload(const Payload& payload) const = 0;

		virtual Length get_width_at_cursor_payload(const Payload& payload) const = 0;
		virtual PagePosition get_position_on_page(const Payload& payload) const = 0;

		virtual Payload new_line(const Payload& payload, Length line_height) = 0;
		virtual Payload move_right(const Payload& payload, Length shift) const = 0;

	protected:
		std::vector<std::unique_ptr<LayoutObject>> m_objects {};
	};

	struct PageLayout : LayoutBase
	{
		explicit PageLayout(Size2d size, Length margin);

		LineCursor newCursor() const;

		std::vector<pdf::Page*> render() const;

	private:
		virtual Payload new_cursor_payload() const override;
		virtual void delete_cursor_payload(Payload& payload) const override;

		virtual Payload copy_cursor_payload(const Payload& payload) const override;
		virtual PagePosition get_position_on_page(const Payload& payload) const override;

		virtual Payload new_line(const Payload& payload, Length line_height) override;
		virtual Payload move_right(const Payload& payload, Length shift) const override;
		virtual Length get_width_at_cursor_payload(const Payload& payload) const override;

	private:
		Size2d m_size;
		Length m_margin;
		size_t m_num_pages = 1;
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

		PagePosition position() const;

	private:
		LayoutBase* m_layout;
		LayoutBase::Payload m_payload;
	};




	// Theoretical fully generic layout idea?
#if 0
	struct LineCursor;

	struct Layout
	{
		// Up to derived classes to static_cast and use
		struct Payload
		{
			char payload[32];
		};

		virtual LineCursor newCursor() = 0;
		virtual Payload copyCursor(Payload) = 0;
		virtual void deleteCursor(Payload) = 0;

		// cursor getters
		virtual Scalar getWidth(Payload) const = 0;

		// cursor mutators
		virtual LineCursor newLine(Payload, Scalar line_height) = 0;
		virtual LineCursor moveRight(Payload, Scalar shift) = 0;
	};

	// A cursor is a line + its position in the line
	// Use .newLine to get a new cursor that corresponds to the new line
	// Use .getWidth to get the width of the line
	// These methods are all forwarded to the underlying Layout, this class exists so we have a copyable handle
	struct LineCursor
	{
		Layout* layout;
		Layout::Payload payload;

		// Make it look copyable but actually it's not that trivial
		LineCursor(const LineCursor& other) : layout(other.layout) { payload = layout->copyCursor(other.payload); }
		LineCursor& operator=(const LineCursor& other)
		{
			auto copy = other;
			std::swap(payload, copy.payload);
			return *this;
		}
		~LineCursor() { layout->deleteCursor(payload); }

		// Actually cursor things
		Scalar getWidth() const { return layout->getWidth(payload); }
		LineCursor newLine(Scalar line_height) const { return layout->newLine(payload, line_height); }
		LineCursor moveRight(Scalar shift) const { return layout->moveRight(payload, shift); }
	};
#endif
}
