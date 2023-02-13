// paragraph.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::layout
{
	struct Line;
}

namespace sap::tree
{
	struct Text : InlineObject
	{
		explicit Text(std::u32string text) : m_contents(std::move(text)) { }
		explicit Text(std::u32string text, const Style* style) : m_contents(std::move(text)) { this->setStyle(style); }

		const std::u32string& contents() const { return m_contents; }
		std::u32string& contents() { return m_contents; }

		std::unique_ptr<Text> clone() const;

	private:
		friend struct layout::Line;

		std::u32string m_contents {};
	};

	struct Separator : InlineObject
	{
		enum SeparatorKind
		{
			SPACE,
			BREAK_POINT,
			HYPHENATION_POINT,
			SENTENCE_END,
		};

		explicit Separator(SeparatorKind kind, int hyphenation_cost = 0)
			: m_kind(kind)
			, m_hyphenation_cost(hyphenation_cost)
		{
			switch(m_kind)
			{
				case decltype(kind)::SPACE:
				case decltype(kind)::SENTENCE_END:
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

		SeparatorKind kind() const { return m_kind; }

		bool isHyphenationPoint() const { return m_kind == HYPHENATION_POINT; }
		bool isExplicitBreakPoint() const { return m_kind == BREAK_POINT; }
		bool isSentenceEnding() const { return m_kind == SENTENCE_END; }
		bool isSpace() const { return m_kind == SPACE; }

		int hyphenationCost() const { return m_hyphenation_cost; }

		std::unique_ptr<Separator> clone() const;

	private:
		friend struct layout::Line;

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

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		virtual ErrorOr<LayoutResult> createLayoutObject(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const override;

		std::vector<std::unique_ptr<InlineObject>>& contents() { return m_contents; }
		const std::vector<std::unique_ptr<InlineObject>>& contents() const { return m_contents; }

		ErrorOr<std::vector<std::unique_ptr<InlineObject>>> processWordSeparators( //
			std::vector<std::unique_ptr<InlineObject>> vec) const;

	private:
		ErrorOr<std::vector<std::unique_ptr<InlineObject>>> evaluate_scripts(interp::Interpreter* cs) const;

	private:
		std::vector<std::unique_ptr<InlineObject>> m_contents {};
	};
}
