// linebreak.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/font.h"
#include "pdf/units.h"

#include "sap/style.h"
#include "sap/units.h"

#include "layout/line.h"
#include "layout/base.h"
#include "layout/word.h"

namespace sap::layout::linebreak
{
	struct BrokenLine
	{
		explicit BrokenLine(const Style& parent_style) : m_parent_style(parent_style) { }

	private:
		Style m_parent_style;

		enum
		{
			NONE,
			WORD,
			SPAN,
		} m_last_obj_kind
		    = NONE;

		Length m_line_height = 0;
		Length m_total_space_width = 0;
		Length m_line_width_excluding_last_word = 0;

		std::u32string m_last_word {};
		const tree::Separator* m_last_sep = nullptr;
		const tree::InlineSpan* m_last_span = nullptr;

		Style m_current_style = Style::empty();

		util::hashmap<size_t, Length> m_adjustments {};

		Length m_left_protrusion = 0;
		Length m_right_protrusion = 0;

		size_t m_num_spaces = 0;
		size_t m_num_parts = 0;

		// Helper functions
		static Size2d calc_word_size(zst::wstr_view text, const Style& style)
		{
			return style.font()->getWordSize(text, style.font_size().into()).into();
		}

		static Size2d calc_span_size(const tree::InlineSpan* span)
		{
			// FIXME: span heights not calculated correctly
			assert(span->hasOverriddenWidth());
			return { *span->getOverriddenWidth(), 0 };
		}

	public:
		Length totalSpaceWidth() const { return m_total_space_width; }
		zst::wstr_view lastWordFragment() const { return m_last_word; }

		Length width()
		{
			auto last_word_style = m_current_style;
			if(m_last_sep != nullptr)
			{
				m_last_word += m_last_sep->endOfLine().sv();

				auto word_width = calc_word_size(m_last_word, last_word_style).x();
				auto ret = m_line_width_excluding_last_word + word_width;

				// if the previous thing wasn't even a word, we have to add its size as well,
				// because it doesn't have text in m_last_word
				if(m_last_obj_kind == SPAN)
					ret += calc_span_size(m_last_span).x();

				m_last_word.erase(m_last_word.size() - m_last_sep->endOfLine().size());
				return ret;
			}
			else
			{
				return m_line_width_excluding_last_word + get_size_of_last_thing(last_word_style).x();
			}
		}

		size_t numParts() const { return m_num_parts; }
		size_t numSpaces() const { return m_num_spaces; }

		Length leftProtrusion() const { return m_left_protrusion; }
		Length rightProtrusion() const { return m_right_protrusion; }

		void setLeftProtrusion(Length len) { m_left_protrusion = len; }
		void setRightProtrusion(Length len) { m_right_protrusion = len; }

		void adjustPiece(size_t idx, Length amount) { m_adjustments[idx] = amount; }
		const util::hashmap<size_t, Length>& pieceAdjustments() const { return m_adjustments; }

		void resetAdjustments()
		{
			m_adjustments.clear();
			m_left_protrusion = 0;
			m_right_protrusion = 0;
		}

		void add(const tree::InlineObject* obj)
		{
			this->_add(obj);
			m_num_parts++;
		}


	private:
		void _add(const tree::InlineObject* obj)
		{
			assert(obj != nullptr);

			if(auto t = obj->castToText())
			{
				// zpr::println("add word ({})", t->contents());
				this->add_word(t);
			}
			else if(auto s = obj->castToSeparator())
			{
				// zpr::println("add sep");
				this->add_sep(s);
			}
			else if(auto ss = obj->castToSpan())
			{
				// zpr::println("add span ({})", ss->canSplit());
				this->add_span(ss);
			}
			else
				sap::internal_error("unsupported: {}", typeid(*obj).name());
		}

		Size2d get_size_of_last_thing(const Style& style)
		{
			if(m_last_obj_kind == WORD)
				return calc_word_size(m_last_word, style);
			else if(m_last_obj_kind == SPAN)
				return calc_span_size(m_last_span);
			else
				return { 0, 0 };
		}

		void add_span(const tree::InlineSpan* span)
		{
			// since we're adding one entire span here, "splittability" isn't the deciding factor.
			if(span->hasOverriddenWidth())
			{
				this->add_span_or_word(span->style());

				m_last_obj_kind = SPAN;
				m_last_span = span;
			}
			else
			{
				for(const auto& obj : span->objects())
					this->_add(obj.get());
			}
		}

		void add_sep(const tree::Separator* sep) { m_last_sep = sep; }

		void add_word(const tree::Text* word)
		{
			auto word_style = m_parent_style.extendWith(word->style());

			auto lh = calc_word_size(word->contents(), word_style).y() * word_style.line_spacing();
			m_line_height = std::max(m_line_height, lh);

			if(bool replace_last_word = this->add_span_or_word(word->style()); replace_last_word)
				m_last_word = word->contents();
			else
				m_last_word += word->contents();

			m_last_obj_kind = WORD;
		}

		bool add_span_or_word(const Style& style)
		{
			auto prev_word_style = m_current_style;
			auto word_style = m_parent_style.extendWith(style);

			bool replace_last_word = false;
			if(m_last_sep != nullptr && m_last_sep->hasWhitespace())
			{
				m_num_spaces++;

				m_line_width_excluding_last_word += get_size_of_last_thing(prev_word_style).x();
				replace_last_word = true;
			}
			else if(m_last_span != nullptr || m_current_style != style)
			{
				m_line_width_excluding_last_word += get_size_of_last_thing(prev_word_style).x();
				replace_last_word = true;
			}
			else
			{
				// either there is no last separator, or the last separator was a hyphen
				replace_last_word = false;
			}

			if(m_last_sep != nullptr)
			{
				auto sep_width = calculatePreferredSeparatorWidth(m_last_sep, /* end of line: */ false, //
				    prev_word_style, word_style);

				m_line_width_excluding_last_word += sep_width;

				if(m_last_sep->hasWhitespace())
					m_total_space_width += sep_width;
			}

			m_current_style = m_parent_style.extendWith(style);
			m_last_span = nullptr;
			m_last_sep = nullptr;

			return replace_last_word;
		}
	};

	std::vector<BrokenLine> breakLines(interp::Interpreter* cs,
	    const Style& parent_style,
	    const std::vector<zst::SharedPtr<tree::InlineObject>>& contents,
	    Length preferred_line_length);
}
