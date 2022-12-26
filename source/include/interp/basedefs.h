#pragma once

#include "sap/style.h" // for Stylable

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
		virtual ~DocumentObject();
	};

	struct InlineObject : Stylable
	{
		virtual ~InlineObject();
	};

	struct BlockObject : DocumentObject
	{
		virtual ~BlockObject();
	};
}
