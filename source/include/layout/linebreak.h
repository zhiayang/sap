// linebreak.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/font.h"  // for Font
#include "pdf/units.h" // for PdfScalar, Size2d_YDown

#include "sap/style.h" // for Style
#include "sap/units.h" // for Length

#include "layout/base.h" // for Size2d, Cursor, RectPageLayout
#include "layout/word.h" // for Separator, Word

namespace sap::layout::linebreak
{
	struct BrokenLine
	{
		BrokenLine(const Style* parent_style, LineCursor m_prev_cursor)
		    : m_parent_style(parent_style)
		    , m_prev_cursor(std::move(m_prev_cursor))
		{
		}

	private:
		// "Input"
		const Style* m_parent_style;
		LineCursor m_prev_cursor;

		// "state" / "output"
		Length m_line_height {};
		Length m_line_width_excluding_last_word {};
		std::optional<Separator> m_last_sep {};
		std::u32string m_last_word {};

		Length m_total_space_width {};

		const Style* m_style = Style::empty();

		size_t m_num_spaces = 0;
		size_t m_num_parts = 0;

		// Helper functions
		static Size2d calculateWordSize(zst::wstr_view text, const Style* style)
		{
			return style->font()->getWordSize(text, style->font_size().into<pdf::PdfScalar>()).into<Size2d>();
		}

	public:
		Length totalSpaceWidth() const { return m_total_space_width; }

		// Getters
		Length width()
		{
			auto last_word_style = m_parent_style->extendWith(m_style);
			if(m_last_sep.has_value())
			{
				m_last_word += m_last_sep->endOfLine().sv();

				auto ret = m_line_width_excluding_last_word + calculateWordSize(m_last_word, last_word_style).x();
				m_last_word.erase(m_last_word.size() - m_last_sep->endOfLine().size());
				return ret;
			}
			else
			{
				return m_line_width_excluding_last_word + calculateWordSize(m_last_word, last_word_style).x();
			}
		}

		size_t numSpaces() const { return m_num_spaces; }
		size_t numParts() const { return m_num_parts; }
		LineCursor lineCursor() const { return m_prev_cursor.newLine(m_line_height); }

		// Modifiers
		void add(const Word& w)
		{
			auto prev_word_style = m_parent_style->extendWith(m_style);
			auto word_style = m_parent_style->extendWith(w.style());

			m_line_height = std::max(m_line_height, calculateWordSize(m_last_word, word_style).y() * word_style->line_spacing());
			m_num_parts++;

			if(m_last_sep.has_value() && m_last_sep->isSpace())
			{
				m_num_spaces++;

				m_line_width_excluding_last_word += calculateWordSize(m_last_word, prev_word_style).x();
				m_last_word.clear();

				m_style = w.style();
				m_last_word = w.text().sv();
			}
			else if(m_style != nullptr && m_style != w.style() && *m_style != *w.style())
			{
				m_line_width_excluding_last_word += calculateWordSize(m_last_word, prev_word_style).x();
				m_last_word.clear();

				m_style = w.style();
				m_last_word = w.text().sv();
			}
			else
			{
				// either there is no last separator, or the last separator was a hyphen
				m_style = w.style();
				m_last_word += w.text().sv();
			}

			if(m_last_sep.has_value())
			{
				auto sep_width = std::max(calculateWordSize(m_last_sep->middleOfLine(), prev_word_style).x(),
				    calculateWordSize(m_last_sep->middleOfLine(), word_style).x());

				m_line_width_excluding_last_word += sep_width;

				if(m_last_sep->isSpace())
					m_total_space_width += sep_width;
			}

			m_last_sep = std::nullopt;
		}

		void add(Separator sep)
		{
			m_num_parts++;
			m_last_sep = std::move(sep);
		}
	};

	std::vector<BrokenLine> breakLines(LayoutBase* layout, LineCursor cursor, const Style* parent_style,
	    const std::vector<std::variant<Word, Separator>>& words, Length preferred_line_length);
}
