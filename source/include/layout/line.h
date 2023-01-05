// line.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <variant> // for variant

#include "pdf/font.h"  // for Font
#include "pdf/units.h" // for PdfScalar, Size2d_YDown

#include "sap/style.h" // for Style
#include "sap/units.h" // for Length

#include "interp/tree.h" // for Separator, Separator::SPACE

#include "layout/base.h" // for Size2d, Cursor, RectPageLayout
#include "layout/word.h" // for Separator, Word

namespace sap::layout
{
	struct Line
	{
		// "Input"
		RectPageLayout* layout;
		const Style* parent_style;
		Cursor prev_cursor;

		// Sometimes we need to populate a line in a struct even though it's never read... yeah i'm sorry :(
		static Line invalid_line() { return Line(); }

		Line(RectPageLayout* layout, const Style* parent_style, Cursor prev_cursor)
		    : layout(layout)
		    , parent_style(parent_style)
		    , prev_cursor(prev_cursor)
		{
		}

	private:
		// "state" / "output"
		Length line_height {};
		Length line_width_excluding_last_word {};
		std::optional<Separator> last_sep {};
		std::u32string last_word {};

		const Style* style = Style::empty();

		size_t num_spaces = 0;
		size_t m_num_parts = 0;

		// Helper functions
		static Size2d calculateWordSize(zst::wstr_view text, const Style* style)
		{
			return style->font()->getWordSize(text, style->font_size().into<pdf::PdfScalar>()).into<Size2d>();
		}

	public:
		// Getters
		Length width()
		{
			auto last_word_style = parent_style->extendWith(style);
			if(last_sep)
			{
				last_word += last_sep->endOfLine().sv();
				auto ret = line_width_excluding_last_word + calculateWordSize(last_word, last_word_style).x();
				last_word.erase(last_word.size() - last_sep->endOfLine().size());
				return ret;
			}
			else
			{
				return line_width_excluding_last_word + calculateWordSize(last_word, last_word_style).x();
			}
		}

		size_t numSpaces() const { return num_spaces; }
		size_t numParts() const { return m_num_parts; }
		Cursor lineCursor() const { return layout->newLineFrom(prev_cursor, line_height); }

		// Modifiers
		void add(const Word& w)
		{
			auto prev_word_style = parent_style->extendWith(style);
			auto word_style = parent_style->extendWith(w.style());
			line_height = std::max(line_height,
			    calculateWordSize(last_word, word_style).y() * word_style->line_spacing().value());

			m_num_parts++;

			if(last_sep)
			{
				if(last_sep->kind == tree::Separator::SPACE)
				{
					line_width_excluding_last_word += calculateWordSize(last_word, prev_word_style).x();
					last_word.clear();

					Length space_width = std::max( //
					    calculateWordSize(last_sep->middleOfLine(), prev_word_style).x(),
					    calculateWordSize(last_sep->middleOfLine(), word_style).x());
					line_width_excluding_last_word += space_width;
					num_spaces++;
				}
				else
				{
					sap::internal_error("support other seps {}", last_sep->kind);
				}

				last_sep.reset();

				style = w.style();
				last_word = w.text().sv();
			}
			else if(style != nullptr && style != w.style() && *style != *w.style())
			{
				line_width_excluding_last_word += calculateWordSize(last_word, prev_word_style).x();
				last_word.clear();
				style = w.style();
				last_word = w.text().sv();
			}
			else
			{
				style = w.style();
				last_word += w.text().sv();
			}
		}

		void add(Separator sep)
		{
			m_num_parts++;

			if(sep.kind == tree::Separator::SPACE)
				last_sep = sep;
			else
				sap::internal_error("support other seps {}", sep.kind);
		}

	private:
		Line() = default;
	};

	std::vector<Line> breakLines(RectPageLayout* layout, Cursor cursor, const Style* parent_style,
	    const std::vector<std::variant<Word, Separator>>& words, Length preferred_line_length);
}
