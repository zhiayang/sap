// layout.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"
#include "types.h" // for GlyphId

#include "pdf/font.h"     // for Font
#include "pdf/page.h"     // for Page
#include "pdf/object.h"   // for Writer
#include "pdf/document.h" // for Document

#include "sap/style.h" // for Stylable, Style
#include "sap/units.h" // for Scalar, Vector2

#include "font/font.h"     // for FontFile, GlyphMetrics
#include "font/features.h" // for GlyphAdjustment

namespace pdf
{
	struct Font;
	struct Page;
	struct Text;
	struct Document;
	struct Writer;
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
	/*
	    This is the (tentative) hierarchy of objects in the layout graph:

	    Word <= Paragraph <-+ LayoutObject <= Page <= Document
	                        |
	            Graphic <---+
	                        |
	            Table <-----+
	                        (etc...)

	    <= is a 'contains' relation; ie. a document contains several pages,
	        a paragraph contains several words

	    <- is a 'is-a' relation; ie. a paragraph is a layout object,
	        a graphic is also a layout object, etc.

	    In this way, the document roughly forms a tree structure. A layout object can contain
	    other objects as children (allowing weird nested things like two-column layouts, miniboxes, etc.).

	    Objects must be contained on a single page (for eg. paragraphs and tables, there is a
	    straightforward idea of "splitting" the object across two or more pages). However, this constraint
	    does not get enforced until the object is laid out on a page (or multiple pages).

	    For actual document creation, the idea of a 'ObjectList' is introduced. One document has exactly
	    one object list, and the object list is just a list of Layout Objects. Since it is very cumbersome
	    to 'know' which page a object will end up in, we just don't care about it.

	    As the document grows, various objects will be added to the object list, but there is no fixed
	    assignment of pages yet (note: we might have special objects that do things like move pages etc.
	    for things like `\pagebreak`).

	    When the document is laid out, the object list is traversed in order. Objects are assigned to
	    LayoutRegions; in the simplest case, each Page contains one region (the drawable area of the page).

	    When the document is rendered, each page is rendered, which in turn renders its region, which
	    renders its contained objects at the specified positions. This process returns the final PDF
	    datastructures, which can then be serialised to a file to generate the final output.
	*/

	struct Page;
	struct Paragraph;
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
		explicit RectPageLayout(Size2d size, Scalar margin) : m_size(size), m_margin(margin) { }

		Cursor newCursor() const { return { SIZE_MAX, m_size }; }
		Scalar getWidthAt(Cursor curs) const
		{
			return curs.page_num == SIZE_MAX ? 0 : m_size.x() - m_margin - curs.pos_on_page.x();
		}
		Cursor newLineFrom(Cursor curs, Scalar line_height)
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
		Cursor moveRightFrom(Cursor curs, Scalar width) const { return { curs.page_num, curs.pos_on_page + Offset2d(width, 0) }; }

		/*
		    Here, `position` is the (absolute) position of the region in the page (since regions cannot nest). This
		    position is then added to the relative positions of each object.
		*/
		std::vector<pdf::Page*> render() const
		{
			std::vector<pdf::Page*> ret;
			for(size_t i = 0; i < m_num_pages; ++i)
			{
				// TODO: Pages should have the size determined by page size
				ret.emplace_back(util::make<pdf::Page>());
			}
			for(auto obj : m_objects)
			{
				obj->render(this, ret);
			}
			return ret;
		}

		void addObject(LayoutObject* obj) { m_objects.push_back(obj); }

	private:
		Size2d m_size;
		Scalar m_margin;
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







	/*
	    for now, the Word acts as the smallest indivisible unit of text in sap; we do not plan to adjust
	    intra-word (ie. letter) spacing at this time.

	    since the Word is used as the unit of typesetting, it needs to hold and *cache* a bunch of important
	    information, notably the bounding box (metrics) and the glyph ids, the former of which can only be
	    computed with the latter.

	    since we need to run ligature substitution and kerning adjustments to determine the bounding box,
	    these will be done on a Word. Again, since letter spacing is not really changing here, these computed
	    values will be cached so less has to be computed on the PDF layer.

	    Words and paragraphs are intertwined; a word is not directly renderable, and must be contained in a
	    paragraph. A paragraph can have as little as one (or in fact zero) words. To this end, the Word
	    struct contains a bunch of information that is used to lay it out within the paragraph (eg. whether
	    the line breaks immediately after).

	    TODO: Make Word cheap to copy
	*/
	struct Word : Stylable
	{
		Word(zst::str_view text, const Style* style);

		/*
		    this assumes that the container (typically a paragraph) has already moved the PDF cursor (ie. wrote
		    some offset commands with TJ or Td), such that this method just needs to output the encoded glyph ids,
		    and any styling effects.
		*/
		void render(pdf::Text* text, Scalar space) const;

		/*
		    returns the width of a space in the font used by this word. This information is only available
		    after `computeMetrics()` is called. Otherwise, it returns 0.
		*/
		Scalar spaceWidth() const { return m_space_width; }
		Size2d size() const { return m_size; }

		struct GlyphInfo
		{
			GlyphId gid;
			font::GlyphMetrics metrics;
			font::GlyphAdjustment adjustments;
		};

		static Word fromTreeText(const tree::Text& w);

	private:
		zst::str_view m_text {};
		Size2d m_size {};

		std::vector<GlyphInfo> m_glyphs {};
		// stuff set by the containing Paragraph during layout and used during rendering.
		Position m_position {};

		// set by computeMetrics;
		Scalar m_space_width {};
	};



	// for now we are not concerned with lines.
	struct Paragraph : LayoutObject
	{
		using LayoutObject::LayoutObject;

		static std::pair<std::optional<const tree::Paragraph*>, Cursor> layout(interp::Interpreter* cs, RectPageLayout* layout,
		    Cursor cursor, const Style* parent_style, const tree::Paragraph* treepara);
		virtual void render(const RectPageLayout* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		std::vector<std::pair<Word, Cursor>> m_words {};
	};




	struct Document : Stylable
	{
		Document();

		Document(const Document&) = delete;
		Document& operator=(const Document&) = delete;

		Document(Document&&) = default;
		Document& operator=(Document&&) = default;

		void addObject(std::unique_ptr<LayoutObject> obj);

		void layout(interp::Interpreter* cs, const tree::Document& document);
		pdf::Font* addFont(font::FontFile* font) { return pdf::Font::fromFontFile(&m_pdf_document, font); }
		void write(pdf::Writer* stream)
		{
			auto pages = m_page_layout.render();
			for(auto& page : pages)
			{
				m_pdf_document.addPage(page);
			}
			m_pdf_document.write(stream);
		}

	private:
		pdf::Document& pdfDocument();
		const pdf::Document& pdfDocument() const;

		pdf::Document m_pdf_document {};
		RectPageLayout m_page_layout = RectPageLayout(dim::Vector2(dim::mm(210), dim::mm(297)).into<Size2d>(), dim::mm(25));

		std::vector<std::unique_ptr<LayoutObject>> m_objects {};
	};
}
