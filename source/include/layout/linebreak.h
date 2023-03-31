// linebreak.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/font.h"  // for Font
#include "pdf/units.h" // for PdfScalar, Size2d_YDown

#include "sap/style.h" // for Style
#include "sap/units.h" // for Length

#include "layout/line.h"
#include "layout/base.h" // for Size2d, Cursor, RectPageLayout
#include "layout/word.h" // for Separator, Word

namespace sap::layout::linebreak
{
	struct BrokenLine
	{
		explicit BrokenLine(const Style* parent_style) : m_parent_style(parent_style) { }

	private:
		const Style* m_parent_style;

		enum
		{
			NONE,
			WORD,
			OTHER,
		} m_last_obj_kind = NONE;

		Length m_line_height = 0;
		Length m_total_space_width = 0;
		Length m_line_width_excluding_last_word = 0;

		std::u32string m_last_word {};
		const tree::Separator* m_last_sep = nullptr;
		const tree::InlineObject* m_last_obj = nullptr;

		const Style* m_current_style = Style::empty();

		size_t m_num_spaces = 0;
		size_t m_num_parts = 0;

		// Helper functions
		static Size2d calculateWordSize(zst::wstr_view text, const Style* style)
		{
			return style->font()->getWordSize(text, style->font_size().into()).into();
		}

	public:
		Length totalSpaceWidth() const { return m_total_space_width; }

		// Getters
		Length width()
		{
			auto last_word_style = m_parent_style->extendWith(m_current_style);
			if(m_last_sep != nullptr)
			{
				m_last_word += m_last_sep->endOfLine().sv();

				auto word_width = calculateWordSize(m_last_word, last_word_style).x();
				auto ret = m_line_width_excluding_last_word + word_width;

				// if the previous thing wasn't even a word, we have to add its size as well,
				// because it doesn't have text in m_last_word
				if(m_last_obj_kind != WORD)
					sap::internal_error("unsupported thing in line");

				m_last_word.erase(m_last_word.size() - m_last_sep->endOfLine().size());
				return ret;
			}
			else
			{
				return m_line_width_excluding_last_word + get_size_of_last_thing(last_word_style).x();
			}
		}

		size_t numSpaces() const { return m_num_spaces; }
		size_t numParts() const { return m_num_parts; }
		Length lineHeight() const { return m_line_height; }

		void add(const tree::InlineObject* obj)
		{
			if(auto w = dynamic_cast<const tree::Text*>(obj); w)
				this->add_word(w);
			else if(auto s = dynamic_cast<const tree::Separator*>(obj); s)
				this->add_sep(s);
			else
				this->add_other(obj);
		}

	private:
		Size2d get_size_of_last_thing(const Style* style)
		{
			if(m_last_obj_kind == WORD)
				return calculateWordSize(m_last_word, style);
			else if(m_last_obj != nullptr)
				sap::internal_error("unsupported");
			else
				return { 0, 0 };
		}

		void add_other(const tree::InlineObject* obj)
		{
			auto& tmp = *obj;
			sap::internal_error("unsupported: {}", typeid(tmp).name());
		}

		// Modifiers
		void add_word(const tree::Text* word)
		{
			auto prev_word_style = m_parent_style->extendWith(m_current_style);
			auto word_style = m_parent_style->extendWith(word->style());

			// TODO: suspicious; should this use `prev_word_style` instead of `word_style`?
			auto this_line_height = calculateWordSize(word->contents(), word_style).y() * word_style->line_spacing();
			m_line_height = std::max(m_line_height, this_line_height);

			if(m_last_sep != nullptr && (m_last_sep->isSpace() || m_last_sep->isSentenceEnding()))
			{
				m_num_spaces++;

				m_line_width_excluding_last_word += get_size_of_last_thing(prev_word_style).x();
				m_last_word = word->contents();
			}
			else if(m_current_style != nullptr && m_current_style != word->style()
					&& *m_current_style != *word->style())
			{
				m_line_width_excluding_last_word += get_size_of_last_thing(prev_word_style).x();
				m_last_word = word->contents();
			}
			else
			{
				// either there is no last separator, or the last separator was a hyphen
				m_last_word += word->contents();
			}

			if(m_last_sep != nullptr)
			{
				auto sep_width = calculatePreferredSeparatorWidth(m_last_sep, /* end of line: */ false, //
					prev_word_style, word_style);

				m_line_width_excluding_last_word += sep_width;

				if(m_last_sep->isSpace() || m_last_sep->isSentenceEnding())
					m_total_space_width += sep_width;
			}

			m_last_sep = nullptr;

			m_current_style = word->style();
			m_last_obj_kind = WORD;
			m_num_parts++;
		}

		void add_sep(const tree::Separator* sep)
		{
			m_num_parts++;
			m_last_sep = sep;
		}
	};

	std::vector<BrokenLine> breakLines(const Style* parent_style,
		const std::vector<std::unique_ptr<tree::InlineObject>>& contents,
		Length preferred_line_length);
}
