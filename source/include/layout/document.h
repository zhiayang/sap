// document.h
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"
#include "types.h" // for GlyphId

#include "pdf/file.h"   // for File
#include "pdf/page.h"   // for Page
#include "pdf/units.h"  // for Scalar
#include "pdf/object.h" // for Writer

#include "sap/style.h" // for Stylable
#include "sap/units.h" // for Vector2
#include "sap/annotation.h"
#include "sap/document_settings.h"

#include "font/font_source.h"

#include "layout/base.h" // for RectPageLayout, LayoutObject, Document

namespace pdf
{
	struct PdfFont;
	struct Page;
	struct Text;
	struct File;
	struct Writer;
}

namespace font
{
	struct FontFile;
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

	struct Document : Stylable
	{
		explicit Document(interp::Interpreter* cs, const DocumentSettings& settings);

		Document(const Document&) = delete;
		Document& operator=(const Document&) = delete;

		Document(Document&&) = default;
		Document& operator=(Document&&) = default;

		void addObject(std::unique_ptr<LayoutObject> obj);

		void write(pdf::Writer* stream);

		PageLayout& pageLayout() { return m_page_layout; }
		const PageLayout& pageLayout() const { return m_page_layout; }

		std::vector<OutlineItem>& outlineItems() { return m_outline_items; }
		const std::vector<OutlineItem>& outlineItems() const { return m_outline_items; }

		void addAnnotation(LinkAnnotation annotation);

		std::vector<LinkAnnotation>& annotations() { return m_annotations; }
		const std::vector<LinkAnnotation>& annotations() const { return m_annotations; }

	private:
		pdf::File& pdf();
		const pdf::File& pdf() const;

		pdf::File m_pdf_document {};
		PageLayout m_page_layout;

		std::vector<OutlineItem> m_outline_items;
		std::vector<LinkAnnotation> m_annotations;

		std::vector<std::unique_ptr<LayoutObject>> m_objects {};
	};
}
