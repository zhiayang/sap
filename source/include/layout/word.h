// word.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/style.h" // for Stylable, Style
#include "sap/units.h" // for Length

#include "interp/tree.h" // for Separator, Separator::SeparatorKind, Separa...

namespace pdf
{
	struct Text;
}

namespace sap::layout
{
	struct Separator : Stylable
	{
		Separator(tree::Separator::SeparatorKind kind, const Style* style) : kind(kind)
		{
			this->setStyle(style);
			switch(kind)
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

		bool isSpace() const { return this->kind == decltype(kind)::SPACE; }
		bool isHyphen() const { return this->kind == decltype(kind)::HYPHENATION_POINT; }

		tree::Separator::SeparatorKind kind;

	private:
		const char32_t* m_end_of_line_char;
		const char32_t* m_middle_of_line_char;

		constexpr static char32_t s_space = U' ';
		constexpr static char32_t s_hyphen = U'-';
	};

	struct Word : Stylable
	{
		Word(zst::wstr_view text, const Style* style);

		/*
		    this assumes that the container (typically a paragraph) has already moved the PDF cursor (ie. wrote
		    some offset commands with TJ or Td), such that this method just needs to output the encoded glyph ids,
		    and any styling effects.
		*/
		void render(pdf::Text* text, Length space) const;

		zst::wstr_view text() const { return m_text; }

	private:
		zst::wstr_view m_text {};
	};
}
