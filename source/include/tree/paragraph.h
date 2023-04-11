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
		explicit Text(std::u32string text);
		explicit Text(std::u32string text, const Style* style);

		const std::u32string& contents() const { return m_contents; }
		std::u32string& contents() { return m_contents; }

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

		explicit Separator(SeparatorKind kind, int hyphenation_cost = 0);

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
		Paragraph() : BlockObject(Kind::Paragraph) { }
		explicit Paragraph(std::vector<zst::SharedPtr<InlineObject>> objs);

		void addObject(zst::SharedPtr<InlineObject> obj);
		void addObjects(std::vector<zst::SharedPtr<InlineObject>> obj);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;

		std::vector<zst::SharedPtr<InlineObject>>& contents() { return m_contents; }
		const std::vector<zst::SharedPtr<InlineObject>>& contents() const { return m_contents; }

		static ErrorOr<std::vector<zst::SharedPtr<InlineObject>>> processWordSeparators( //
		    std::vector<zst::SharedPtr<InlineObject>> vec);

		using EvalScriptResult = zst::Either<zst::SharedPtr<InlineSpan>, std::unique_ptr<layout::LayoutObject>>;
		ErrorOr<std::optional<EvalScriptResult>> evaluate_scripts(interp::Interpreter* cs,
		    Size2d available_space) const;

	private:
		ErrorOr<std::optional<EvalScriptResult>> eval_single_script_in_para(interp::Interpreter* cs,
		    Size2d available_space,
		    ScriptCall* script,
		    bool allow_blocks) const;

		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
		    const Style* parent_style,
		    Size2d available_space) const override;

	private:
		std::vector<zst::SharedPtr<InlineObject>> m_contents {};
	};
}
