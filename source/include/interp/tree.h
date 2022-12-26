// tree.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/style.h" // for Style

#include "interp/interp.h"   // for Stmt, FunctionCall, Interpreter
#include "interp/basedefs.h" // for InlineObject, DocumentObject, BlockObject


// pun not intended
namespace sap::tree
{
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
		explicit Text(std::u32string text) : m_contents(std::move(text)) { }
		explicit Text(std::u32string text, const Style* style) : m_contents(std::move(text)) { this->setStyle(style); }

		const std::u32string& contents() const { return m_contents; }
		std::u32string& contents() { return m_contents; }

	private:
		std::u32string m_contents {};
	};

	struct Separator : InlineObject
	{
		enum SeparatorKind
		{
			SPACE,
			BREAK_POINT,
			HYPHENATION_POINT,
		};

		explicit Separator(SeparatorKind kind) : m_kind(kind) { }

		SeparatorKind kind() const { return m_kind; }

	private:
		SeparatorKind m_kind;
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
