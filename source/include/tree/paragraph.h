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

		explicit Separator(SeparatorKind kind, int hyphenation_cost = 0) : m_kind(kind), m_hyphenation_cost(hyphenation_cost)
		{
			switch(m_kind)
			{
				case decltype(kind)::SPACE:
					m_end_of_line_char = 0;
					m_middle_of_line_char = &s_space;
					break;

				case decltype(kind)::BREAK_POINT:
					m_end_of_line_char = 0;
					m_middle_of_line_char = 0;
					break;

				case decltype(kind)::HYPHENATION_POINT:
					m_end_of_line_char = &s_hyphen;
					m_middle_of_line_char = 0;
					break;
			}
		}


		zst::wstr_view endOfLine() const
		{
			return m_end_of_line_char == 0 ? zst::wstr_view() : zst::wstr_view(m_end_of_line_char, 1);
		}

		zst::wstr_view middleOfLine() const
		{
			return m_middle_of_line_char == 0 ? zst::wstr_view() : zst::wstr_view(m_middle_of_line_char, 1);
		}

		bool isSpace() const { return m_kind == SPACE; }
		bool isHyphenationPoint() const { return m_kind == HYPHENATION_POINT; }
		bool isExplicitBreakPoint() const { return m_kind == BREAK_POINT; }

		int hyphenationCost() const { return m_hyphenation_cost; }

	private:
		const char32_t* m_end_of_line_char;
		const char32_t* m_middle_of_line_char;
		SeparatorKind m_kind;
		int m_hyphenation_cost;

		constexpr static char32_t s_space = U' ';
		constexpr static char32_t s_hyphen = U'-';
	};

	struct Paragraph : BlockObject
	{
		Paragraph() = default;
		explicit Paragraph(std::vector<std::unique_ptr<InlineObject>> objs);

		void addObject(std::unique_ptr<InlineObject> obj);
		void addObjects(std::vector<std::unique_ptr<InlineObject>> obj);

		virtual LayoutResult createLayoutObject(interp::Interpreter* cs, layout::LineCursor cursor, const Style* parent_style)
		    const override;

		std::vector<std::unique_ptr<InlineObject>>& contents() { return m_contents; }
		const std::vector<std::unique_ptr<InlineObject>>& contents() const { return m_contents; }

	private:
		std::vector<std::unique_ptr<InlineObject>> m_contents {};

		friend struct Document;
		void evaluateScripts(interp::Interpreter* cs);
		void processWordSeparators();
	};
}
