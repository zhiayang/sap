// tree.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/style.h" // for Style

#include "interp/ast.h"      // for Stmt, FunctionCall
#include "interp/value.h"    // for Interpreter
#include "interp/basedefs.h" // for InlineObject, DocumentObject, BlockObject


// pun not intended
namespace sap::tree
{
	struct Document
	{
		void addObject(std::unique_ptr<DocumentObject> obj);

		std::vector<DocumentObject*>& objects() { return m_objects; }
		const std::vector<DocumentObject*>& objects() const { return m_objects; }

		void evaluateScripts(interp::Interpreter* cs);
		void processWordSeparators();

	private:
		std::vector<DocumentObject*> m_objects {};
		std::vector<std::unique_ptr<DocumentObject>> m_all_objects {};
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

		explicit Separator(SeparatorKind kind, int hyphenation_cost = 0) //
		    : m_kind(kind)
		    , m_hyphenation_cost(hyphenation_cost)
		{
		}

		SeparatorKind kind() const { return m_kind; }
		int hyphenationCost() const { return m_hyphenation_cost; }

	private:
		SeparatorKind m_kind;
		int m_hyphenation_cost;
	};

	struct Paragraph : BlockObject
	{
		void addObject(std::unique_ptr<InlineObject> obj);
		void addObjects(std::vector<std::unique_ptr<InlineObject>> obj);

		virtual std::optional<LayoutFn> getLayoutFunction() const override;

		std::vector<std::unique_ptr<InlineObject>>& contents() { return m_contents; }
		const std::vector<std::unique_ptr<InlineObject>>& contents() const { return m_contents; }

	private:
		std::vector<std::unique_ptr<InlineObject>> m_contents {};

		friend struct Document;
		void evaluateScripts(interp::Interpreter* cs);
		void processWordSeparators();
	};

	struct Image : BlockObject
	{
		explicit Image(OwnedImageBitmap image, sap::Vector2 size) : m_image(std::move(image)), m_size(size) { }

		virtual std::optional<LayoutFn> getLayoutFunction() const override;

		static std::unique_ptr<Image> fromImageFile(zst::str_view file_path, sap::Length width,
		    std::optional<sap::Length> height = std::nullopt);

		ImageBitmap image() const { return m_image.span(); }
		sap::Vector2 size() const { return m_size; }

	private:
		OwnedImageBitmap m_image;
		sap::Vector2 m_size;
	};


	/*
	    Since inline objects are not document objects, we do not have a diamond problem by
	    inheriting both of them. We want this hierarchy because script blocks and calls can
	    appear both at the top-level (with blocks or floats), and within blocks/floats as well.
	*/
	struct ScriptObject : InlineObject, DocumentObject
	{
		virtual std::optional<LayoutFn> getLayoutFunction() const override;
	};

	struct ScriptBlock : ScriptObject
	{
		std::unique_ptr<interp::Block> body;
	};

	struct ScriptCall : ScriptObject
	{
		std::unique_ptr<interp::FunctionCall> call;
	};
}
