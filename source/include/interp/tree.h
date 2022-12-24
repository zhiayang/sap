// tree.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdint.h>
#include <stddef.h>

#include <memory>

#include "sap/style.h"
#include "interp/interp.h"


// pun not intended
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

		/*
		    these two control whether there should be a space before/after the
		    result of the object. for example:

		    foo\bold("bar")aoeu should give us 'foobaraoeu',
		    foo\bold("bar") aoeu should give 'foobar aoeu', etc.
		*/
		bool stick_to_left = false;
		bool stick_to_right = false;
	};

	struct BlockObject : DocumentObject
	{
		virtual ~BlockObject();
	};

	struct Document
	{
		void addObject(std::shared_ptr<DocumentObject> obj);

		std::vector<std::shared_ptr<DocumentObject>>& objects() { return m_objects; }
		const std::vector<std::shared_ptr<DocumentObject>>& objects() const { return m_objects; }

		void evaluateScripts(interp::Interpreter* cs);
		void processWordSeparators();

	private:
		std::vector<std::shared_ptr<DocumentObject>> m_objects {};
	};






	struct Text : InlineObject
	{
		explicit Text(std::string text) : m_contents(std::move(text)) { }
		explicit Text(std::string text, const Style* style) : m_contents(std::move(text)) { this->setStyle(style); }

		const std::string& contents() const { return m_contents; }
		std::string& contents() { return m_contents; }

	private:
		std::string m_contents {};
	};

	struct Separator : InlineObject
	{
	};

	struct Paragraph : BlockObject
	{
		void addObject(std::shared_ptr<InlineObject> obj);

		std::vector<std::shared_ptr<InlineObject>>& contents() { return m_contents; }
		const std::vector<std::shared_ptr<InlineObject>>& contents() const { return m_contents; }

	private:
		std::vector<std::shared_ptr<InlineObject>> m_contents {};

		friend Document;
		void evaluateScripts(interp::Interpreter* cs);
		void processWordSeparators();
	};




	/*
	    Since inline objects are not document objects, we do not have a diamond problem by
	    inheriting both of them. We want this hierarchy because script blocks and calls can
	    appear both at the top-level (with blocks or floats), and within blocks/floats as well.
	*/
	struct ScriptObject : InlineObject, DocumentObject
	{
	};

	struct ScriptBlock : ScriptObject
	{
		std::vector<std::unique_ptr<interp::Stmt>> statements;
	};

	struct ScriptCall : ScriptObject
	{
		std::unique_ptr<interp::FunctionCall> call;
	};


}
