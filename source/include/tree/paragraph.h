// paragraph.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
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
		Paragraph() = default;
		explicit Paragraph(std::vector<std::unique_ptr<InlineObject>> objs);

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
}
