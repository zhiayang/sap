// paragraph.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <variant> // for variant, visit

#include "util.h" // for dynamic_pointer_cast

#include "pdf/page.h"  // for Page
#include "pdf/text.h"  // for Text
#include "pdf/units.h" // for Position2d_YDown, Scalar, Offset2d

#include "sap/style.h"    // for Style
#include "sap/units.h"    // for Scalar
#include "sap/document.h" // for Word, Cursor, Paragraph, RectPageLayout

#include "interp/tree.h" // for Paragraph, Text
#include "zst.h"

namespace sap::layout
{
	struct Separator : Stylable
	{
		Separator(tree::Separator::SeparatorKind kind, const Style* style)
		{
			this->setStyle(style);
			switch(kind)
			{
				case decltype(kind)::SPACE:
					m_end_of_line_char = 0;
					m_middle_of_line_char = U' ';
					break;

				case decltype(kind)::BREAK_POINT:
					m_end_of_line_char = 0;
					m_middle_of_line_char = 0;
					break;

				case decltype(kind)::HYPHENATION_POINT:
					m_end_of_line_char = U'-';
					m_middle_of_line_char = 0;
					break;
			}
		}

		zst::wstr_view endOfLine() const
		{
			return m_end_of_line_char == 0 ? zst::wstr_view() : zst::wstr_view(&m_end_of_line_char, 1);
		}

		zst::wstr_view middleOfLine() const
		{
			return m_middle_of_line_char == 0 ? zst::wstr_view() : zst::wstr_view(&m_middle_of_line_char, 1);
		}

	private:
		char32_t m_end_of_line_char;
		char32_t m_middle_of_line_char;
	};

	static Scalar get_word_width(zst::wstr_view string, const Style* style)
	{
		static util::hashmap<std::u32string, Scalar> s_cache {};
		if(auto it = s_cache.find(string); it != s_cache.end())
			return it->second;

		auto font = style->font_set().getFontForStyle(style->font_style());
		auto font_size = style->font_size();

		Scalar width = 0;
		auto glyphs = Word::getGlyphInfosForString(string, style);
		for(auto& g : glyphs)
		{
			auto tmp = g.metrics.horz_advance + g.adjustments.horz_advance;
			width += font->scaleMetricForFontSize(tmp, font_size.into<pdf::Scalar>()).into<sap::Scalar>();
		}

		s_cache[string.str()] = width;
		return width;
	}

	using WordVecIter = std::vector<Word>::const_iterator;

	[[maybe_unused]] static std::vector<WordVecIter> break_lines(const std::vector<Word>& words, Scalar preferred_line_length)
	{
		struct LineBreakNode
		{
			const std::vector<Word>* words;
			Scalar preferred_line_length;
			WordVecIter broken_until;
			WordVecIter end;

			using Distance = double;

			std::vector<std::pair<LineBreakNode, Distance>> neighbours()
			{
				Scalar cur_line_length;
				std::vector<std::pair<LineBreakNode, Distance>> ret;

				WordVecIter it = broken_until;

				// TODO: Handle lines that only consist of 1 word
				//       This involves tweaking the cost computation
				// TODO: Handle splitting a singular very long word into multiple words forcefully
				if(std::distance(it, end) < 2)
					sap::internal_error("Kill die me now because not yet implemented");


				// Consume 2 words because there are at least 2 words remaining
				cur_line_length += it->size().x();
				++it;
				auto prev_space = it->spaceWidth();
				cur_line_length += it->size().x();
				++it;

				while(true)
				{
					LineBreakNode node { words, preferred_line_length, it, end };

					// Cost calculation:
					// for now, we just do square of the diff between
					//   preferred space length: cur_line_length / (words - 1)
					//   and actual space length: preferred_line_length / (words - 1)
					//
					// special case for last line: no cost!
					if(it == end)
					{
						ret.emplace_back(node, Distance {});
						break;
					}

					auto pref_space_len = cur_line_length.value() / static_cast<double>(it - broken_until - 1);
					auto actual_space_len = preferred_line_length.value() / static_cast<double>(it - broken_until - 1);
					auto diff = pref_space_len - actual_space_len;
					ret.emplace_back(node, diff * diff);

					// Consume a word
					cur_line_length += it->size().x();
					auto next_space = it->spaceWidth();
					auto space_between = dim::max(prev_space, next_space);
					cur_line_length += space_between;
					prev_space = next_space;
					++it;

					// Do not consider this word if we're too long
					if(cur_line_length > preferred_line_length)
					{
						break;
					}
				}

				return ret;
			}

			size_t hash() const { return static_cast<size_t>(broken_until - words->begin()); }
			bool operator==(LineBreakNode other) const { return broken_until == other.broken_until; }
		};

		/* auto path = util::dijkstra_shortest_path(LineBreakNode { &words, preferred_line_length, words.begin(), words.end() },
		 */
		std::vector<LineBreakNode> path {};
		std::vector<WordVecIter> path_as_iters;
		for(const LineBreakNode& node : path)
		{
			path_as_iters.push_back(node.broken_until);
		}
		return path_as_iters;
	}

	std::pair<std::optional<const tree::Paragraph*>, Cursor> Paragraph::layout(interp::Interpreter* cs, RectPageLayout* layout,
	    Cursor cursor, const Style* parent_style, const tree::Paragraph* treepara)
	{
		cursor = layout->newLineFrom(cursor, 0);

		using WordSepVec = std::vector<std::variant<Word, Separator>>;
		WordSepVec words_and_seps;

		for(auto& wordorsep : treepara->contents())
		{
			auto style = Style::combine(wordorsep->style(), parent_style);
			if(auto word = util::dynamic_pointer_cast<tree::Text>(wordorsep); word != nullptr)
				words_and_seps.push_back(Word(word->contents(), style));

			else if(auto sep = util::dynamic_pointer_cast<tree::Separator>(wordorsep); sep != nullptr)
				words_and_seps.push_back(Separator(sep->kind(), sep->style()));
		}

		auto para = util::make<Paragraph>();

		WordSepVec cur_line;
		Scalar prev_space;
		Scalar cur_line_length;
		Scalar cur_line_spacing;

		struct Line
		{
			std::vector<std::variant<Word, Separator>> words;
			Cursor cursor;
			Scalar line_length;
		};

		// Break lines
		std::vector<Line> lines;
		for(auto& wordorsep : words_and_seps)
		{
			auto word_visitor = [&](const Word& word) {
				auto new_line_length = cur_line_length + prev_space + word.size().x();
				if(new_line_length >= layout->getWidthAt(cursor))
				{
					cursor = layout->newLineFrom(cursor, cur_line_spacing);
					lines.push_back(Line {
					    .words = std::move(cur_line),
					    .cursor = cursor,
					    .line_length = cur_line_length,
					});
					cur_line.clear();

					cursor = layout->newLineFrom(cursor, 0);
					cur_line_length = word.size().x();
				}
				else
				{
					cur_line_length = new_line_length;
				}

				prev_space = word.spaceWidth();
				cur_line_spacing = std::max(cur_line_spacing, word.size().y() + word.style()->line_spacing());

				cur_line.push_back(word);
			};

			auto sep_visitor = [&](const Separator& sep) {

			};

			std::visit(util::overloaded { word_visitor, sep_visitor }, wordorsep);
		}

		cursor = layout->newLineFrom(cursor, cur_line_spacing);
		lines.push_back(Line {
		    .words = std::move(cur_line),
		    .cursor = cursor,
		    .line_length = cur_line_length,
		});
		cur_line.clear();

		cursor = layout->newLineFrom(cursor, 0);

		// Justify and add words to region
		for(auto it = lines.begin(); it != lines.end(); ++it)
		{
			auto& line = *it;

			cursor = line.cursor;
			auto total_extra_space_width = layout->getWidthAt(cursor) - line.line_length;
			auto num_spaces = line.words.size() - 1; // TODO: this should be = the # of separators
			auto extra_space_width = num_spaces == 0 ? 0 : total_extra_space_width / (double) num_spaces;

			if(it + 1 == lines.end())
				extra_space_width = 0;

			for(auto& wordorsep : line.words)
			{
				auto word_visitor = [&](const Word& word) {
					para->m_words.emplace_back(word, cursor);

					// TODO: add space in separate case but for now every word is followed by a space
					cursor = layout->moveRightFrom(cursor, word.size().x());
					cursor = layout->moveRightFrom(cursor, word.spaceWidth());
					cursor = layout->moveRightFrom(cursor, extra_space_width);
				};

				auto sep_visitor = [&](const Separator& sep) {};

				std::visit(util::overloaded { word_visitor, sep_visitor }, wordorsep);
			}
		}

		layout->addObject(para);
		return { std::nullopt, cursor };
	}


	void Paragraph::render(const RectPageLayout* layout, std::vector<pdf::Page*>& pages) const
	{
		if(m_words.empty())
		{
			return;
		}

		Cursor current_curs = m_words.front().second;
		pdf::Text* cur_text = util::make<pdf::Text>();
		cur_text->moveAbs(pages[m_words.front().second.page_num]->convertVector2(
		    m_words.front().second.pos_on_page.into<pdf::Position2d_YDown>()));

		for(size_t i = 0; i < m_words.size(); i++)
		{
			/*
			    not sure if this is legit, but words basically don't actually use their own `m_position` when
			    rendering; we just pass the words to the PDF, and the viewer uses the font metrics to adjust.

			    we do use the computed position when advancing to the next line, to account for line spacing
			    differences if there's more than one font-size on a line.
			*/
			auto space = m_words[i].second.pos_on_page.x() - current_curs.pos_on_page.x();

			if(current_curs.page_num != m_words[i].second.page_num)
			{
				pages[m_words[i - 1].second.page_num]->addObject(cur_text);
				cur_text = util::make<pdf::Text>();
				cur_text->moveAbs(pages[m_words[i].second.page_num]->convertVector2(
				    m_words[i].second.pos_on_page.into<pdf::Position2d_YDown>()));
				space = 0;
			}
			else if(current_curs.pos_on_page.y() != m_words[i].second.pos_on_page.y())
			{
				auto skip = m_words[i].second.pos_on_page.y() - current_curs.pos_on_page.y();
				cur_text->nextLine(pdf::Offset2d(0, -1.0 * skip.into<pdf::Scalar>().value()));
				space = 0;
			}

			m_words[i].first.render(cur_text, space);

			current_curs = m_words[i].second;
			current_curs.pos_on_page.x() += m_words[i].first.size().x();
		}
		pages[m_words.back().second.page_num]->addObject(cur_text);
	}
}
