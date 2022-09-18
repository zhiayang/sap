// layout.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>

#include <vector>
#include <utility>

#include <zst.h>

#include "defs.h"
#include "font/font.h"
#include "sap/style.h"
#include "pdf/document.h"

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
	struct LayoutRegion;

	/*
	    As mentioned in the overview above, a LayoutObject is some object that can be laid out and
	    rendered into the document.
	*/
	struct LayoutObject : Stylable
	{
		virtual ~LayoutObject() { }

		explicit inline LayoutObject() : m_parent(nullptr) { }
		explicit inline LayoutObject(LayoutObject* parent) : m_parent(parent) { }

		inline LayoutObject* parent() { return m_parent; }
		inline const LayoutObject* parent() const { return m_parent; }

		inline void setParent(LayoutObject* parent) { m_parent = parent; }

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
		virtual zst::Result<std::optional<LayoutObject*>, int> layout(interp::Interpreter* cs, LayoutRegion* region,
			const Style* parent_style) = 0;

		/*
		    Render (emit PDF commands) the object. Must be called after layout(). For now, we render directly to
		    the PDF page (by construcitng and emitting PageObjects), instead of returning a pageobject -- some
		    layout objects might require multiple pdf page objects, so this is a more flexible design.
		*/
		virtual void render(interp::Interpreter* cs, const LayoutRegion* region, Position position, pdf::Page* page) const = 0;

	private:
		LayoutObject* m_parent = nullptr;
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
	struct LayoutRegion
	{
		explicit LayoutRegion(Size2d size);

		void moveCursorTo(Position pos);
		void advanceCursorBy(Offset2d offset);

		void addObjectAtCursor(LayoutObject* obj);
		void addObjectAtPosition(LayoutObject* obj, Position pos);

		Position cursor() const;
		Size2d spaceAtCursor() const;
		bool haveSpaceAtCursor(Size2d size) const;

		/*
		    Here, `position` is the (absolute) position of the region in the page (since regions cannot nest). This
		    position is then added to the relative positions of each object.
		*/
		void render(interp::Interpreter* cs, Position position, pdf::Page* page) const;

	private:
		Size2d m_size {};

		Position m_cursor {};
		std::vector<std::pair<Position, LayoutObject*>> m_objects {};
	};







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
	*/
	struct Word : Stylable
	{
		explicit inline Word(int kind) : kind(kind) { }

		inline Word(int kind, zst::str_view sv) : Word(kind, sv.str()) { }
		inline Word(int kind, std::string str) : kind(kind), text(std::move(str)) { }

		// this fills in the `size` and the `glyphs`
		void computeMetrics(const Style* parent_style);

		/*
		    this assumes that the container (typically a paragraph) has already moved the PDF cursor (ie. wrote
		    some offset commands with TJ or Td), such that this method just needs to output the encoded glyph ids,
		    and any styling effects.
		*/
		void render(pdf::Text* text) const;

		/*
		    returns the width of a space in the font used by this word. This information is only available
		    after `computeMetrics()` is called. Otherwise, it returns 0.
		*/
		Scalar spaceWidth() const;

		int kind = 0;
		std::string text {};

		Size2d size {};

		// the kind of word. this affects automatic handling of certain things during paragraph
		// layout, eg. whether or not to insert a space (or how large of a space).
		static constexpr int KIND_LATIN = 0;
		static constexpr int KIND_CJK = 1;
		static constexpr int KIND_PUNCT = 2;

		friend struct Paragraph;

		struct GlyphInfo
		{
			GlyphId gid;
			font::GlyphMetrics metrics;
			font::GlyphAdjustment adjustments;
		};

	private:
		const Paragraph* m_paragraph = nullptr;

		// first element is the glyph id, second one is the adjustment to make for kerning (0 if none)
		std::vector<GlyphInfo> m_glyphs {};

		// stuff set by the containing Paragraph during layout and used during rendering.
		Position m_position {};
		Word* m_next_word = nullptr;
		bool m_linebreak_after = false;
		double m_post_space_ratio {};

		// set by computeMetrics;
		Scalar m_space_width {};
	};


	// for now we are not concerned with lines.
	struct Paragraph : LayoutObject
	{
		void add(Word word);

		virtual zst::Result<std::optional<LayoutObject*>, int> layout(interp::Interpreter* cs, LayoutRegion* region,
			const Style* parent_style) override;
		virtual void render(interp::Interpreter* cs, const LayoutRegion* region, Position position,
			pdf::Page* page) const override;

	private:
		std::vector<Word> m_words {};
	};


	struct Page : Stylable
	{
		explicit Page(Size2d paper_size);

		Page(const Page&) = delete;
		Page& operator=(const Page&) = delete;

		Page(Page&&) = default;
		Page& operator=(Page&&) = default;

		inline LayoutRegion* layoutRegion() { return &m_layout_region; }
		inline const LayoutRegion* layoutRegion() const { return &m_layout_region; }

		pdf::Page* render(interp::Interpreter* cs);

	private:
		LayoutRegion m_layout_region;
	};




	struct Document : Stylable
	{
		Document();

		Document(const Document&) = delete;
		Document& operator=(const Document&) = delete;

		Document(Document&&) = default;
		Document& operator=(Document&&) = default;

		pdf::Document& pdfDocument();
		const pdf::Document& pdfDocument() const;

		void addObject(std::unique_ptr<LayoutObject> obj);

		void layout(interp::Interpreter* cs);
		pdf::Document& render(interp::Interpreter* cs);

	private:
		pdf::Document m_pdf_document {};
		std::vector<Page> m_pages {};

		std::vector<std::unique_ptr<LayoutObject>> m_objects {};
	};

	Document createDocumentLayout(interp::Interpreter* cs, tree::Document& document);
}
