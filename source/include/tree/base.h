// base.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/units.h"
#include "sap/style.h" // for Stylable

namespace sap::layout
{
	struct PageCursor;
	struct LayoutBase;
	struct LayoutObject;
}

namespace sap::interp
{
	struct Block;
	struct Interpreter;
	struct FunctionCall;
}

namespace sap::tree
{
	/*
	    This defines the main tree structure for the entire system. This acts as the "AST" of the entire
	    document, and is what the metaprogram will traverse/manipulate (in the future).

	    Borrowing a little terminology from CSS, we have InlineObjects, FloatObjects, and BlockObjects.
	    Inline objects are simple -- they are words, maths expressions, or anything that should be laid
	    out "inline" within a paragraph.

	    Float objects and Block objects are similar, except that floats can be reordered (ie. the float
	    environment in LaTeX), while blocks cannot. Inline objects can only appear inside a float or
	    a block, and not at the top level.

	    We roughly follow the hierarchical layout of a document, starting with the top level Document,
	    containing some DocumentObjects, each being either a "tangible" object like a float (eg. image)
	    or block (eg. paragraph), or an "intangible" one like a script block.

	    // TODO:
	    for now, we do not handle paging, since that is only handled after layout. To correctly support
	    post-layout processing by the metaprogram, paging information needs to be backfed into this tree
	    after layout.
	*/

	struct DocumentObject
	{
		virtual ~DocumentObject() = 0;

		using LayoutResult = std::pair<layout::PageCursor, std::vector<std::unique_ptr<layout::LayoutObject>>>;

		virtual LayoutResult createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style)
		    const = 0;
	};

	struct InlineObject : Stylable
	{
		virtual ~InlineObject() = 0;
		Size2d size() const { return m_size; }

	protected:
		Size2d m_size { 0, 0 };
	};

	struct BlockObject : Stylable, DocumentObject
	{
		Size2d size() const { return m_size; }

	protected:
		Size2d m_size { 0, 0 };
	};

	/*
	    Since inline objects are not document objects, we do not have a diamond problem by
	    inheriting both of them. We want this hierarchy because script blocks and calls can
	    appear both at the top-level (with blocks or floats), and within blocks/floats as well.
	*/
	struct ScriptObject : InlineObject, BlockObject
	{
	};

	struct ScriptBlock : ScriptObject
	{
		virtual LayoutResult createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style)
		    const override;

		std::unique_ptr<interp::Block> body;
	};

	struct ScriptCall : ScriptObject
	{
		virtual LayoutResult createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style)
		    const override;

		std::unique_ptr<interp::FunctionCall> call;
	};
}
