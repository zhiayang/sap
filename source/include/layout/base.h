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
	struct RectPageLayout;

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
		    Place (lay out) the object into the given region, using the `parent_style` as a fallback style
		    if necessary during the layout process.

		    If the result is a success, it may still return an optional LayoutObject; this indicates that
		    the object could not be fully laid out on the current page. In that case, the remainder is
		    returned (in the optional), and is expected to be laid out on the following page.

		    Note that this process can repeat; the remainder can itself have a remainder, so the initial
		    object might have spanned 3 or more pages.

		    If the result is an error, then the object could not be laid out on the given page, *and*
		    it could not be split (eg. images, graphics). In this case, placement should continue on the
		    subsequent page.
		*/

		// LayoutObjects should implement this
		// static zst::Result<std::optional<LayoutObject*>, int> layout(interp::Interpreter* cs, LayoutRegion* region,
		//     const Style* parent_style);

		/*
		    Render (emit PDF commands) the object. Must be called after layout(). For now, we render directly to
		    the PDF page (by construcitng and emitting PageObjects), instead of returning a pageobject -- some
		    layout objects might require multiple pdf page objects, so this is a more flexible design.
		*/
		virtual void render(const RectPageLayout* layout, std::vector<pdf::Page*>& pages) const = 0;

	private:
		std::vector<std::unique_ptr<LayoutObject>> m_children {};
	};


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

		void addObject(LayoutObject* obj) { m_objects.push_back(obj); }

	private:
		Size2d m_size;
		Length m_margin;
		size_t m_num_pages = 1;
		std::vector<LayoutObject*> m_objects {};
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