// paragraph.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <variant>

#include "sap.h"

#include "util.h"
#include "dijkstra.h"

#include "pdf/page.h"
#include "pdf/font.h"
#include "pdf/text.h"
#include "pdf/units.h"
#include "interp/tree.h"

namespace sap::layout
{

	using WordVecIter = std::vector<Word>::const_iterator;

	struct Line
	{
		std::vector<Word> words;
		bool is_hyphenated = false;
	};

	static std::vector<WordVecIter> break_lines(const std::vector<Word>& words, Scalar preferred_line_length)
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
				{
					sap::internal_error("Kill die me now because not yet implemented");
				}

				// Consume 2 words because there are more than 2 words remaining
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
					//   preferred space length (cur_line_length / (words - 1))
					//   and actual space length (preferred_line_length / (words - 1)
					//
					// special case for last line: no cost!
					if(it == end)
					{
						ret.emplace_back(node, Distance {});
						break;
					}
					auto pref_space_len = cur_line_length.value() / static_cast<double>(it - broken_until - 1);
					auto actual_space_len = preferred_line_length.value() / static_cast<double>(it - broken_until - 1);
					ret.emplace_back(node, (pref_space_len - actual_space_len) * (pref_space_len - actual_space_len));

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
		cursor = layout->newLineFrom(cursor, Scalar());

		using WordSepVec = std::vector<std::variant<Word>>;
		WordSepVec words_and_seps;
		for(auto wordorsep : treepara->contents())
		{
			if(auto word = util::dynamic_pointer_cast<tree::Text>(wordorsep); word != nullptr)
			{
				words_and_seps.push_back(Word(word->contents(), Style::combine(word->style(), parent_style)));
			}
		}

		auto para = util::make<Paragraph>();
		WordSepVec cur_line;
		Scalar prev_space;
		Scalar cur_line_length;
		Scalar cur_line_spacing;

		// Break lines
		std::vector<std::tuple<std::vector<std::variant<Word>>, Cursor, Scalar>> lines;
		for(auto wordorsep : words_and_seps)
		{
			std::visit(
			    [&](Word word) {
				    cur_line_length += prev_space;
				    cur_line_length += word.size().x();
				    if(cur_line_length >= layout->getWidthAt(cursor))
				    {
					    cur_line_length -= prev_space;
					    cur_line_length -= word.size().x();
					    cursor = layout->newLineFrom(cursor, cur_line_spacing);
					    lines.emplace_back(std::move(cur_line), cursor, cur_line_length);
					    cur_line.clear();
					    cursor = layout->newLineFrom(cursor, Scalar());
					    cur_line_length = word.size().x();
				    }
				    prev_space = word.spaceWidth();
				    cur_line_spacing = std::max(cur_line_spacing, word.size().y() + word.style()->line_spacing());

				    cur_line.push_back(word);
			    },
			    wordorsep);
		}
		cursor = layout->newLineFrom(cursor, cur_line_spacing);
		lines.emplace_back(std::move(cur_line), cursor, cur_line_length);
		cur_line.clear();
		cursor = layout->newLineFrom(cursor, Scalar());

		// Justify and add words to region
		for(auto it = lines.begin(); it != lines.end(); ++it)
		{
			const auto& [line, line_cursor, line_length] = *it;

			cursor = line_cursor;
			auto total_extra_space_width = layout->getWidthAt(cursor) - line_length;
			auto num_spaces = line.size() - 1; // TODO: this should be = the # of separators
			auto extra_space_width = num_spaces == 0 ? Scalar() : total_extra_space_width / (double) num_spaces;
			if(it + 1 == lines.end())
			{
				extra_space_width = Scalar();
			}
			for(auto wordorsep : line)
			{
				std::visit(
				    [&](Word word) {
					    para->m_words.emplace_back(word, cursor);
					    // TODO: add space in separate case but for now every word is followed by a space
					    cursor = layout->moveRightFrom(cursor, word.size().x());
					    cursor = layout->moveRightFrom(cursor, word.spaceWidth());
					    cursor = layout->moveRightFrom(cursor, extra_space_width);
				    },
				    wordorsep);
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
				space = Scalar();
			}
			else if(current_curs.pos_on_page.y() != m_words[i].second.pos_on_page.y())
			{
				auto skip = m_words[i].second.pos_on_page.y() - current_curs.pos_on_page.y();
				cur_text->nextLine(pdf::Offset2d(0, -1.0 * skip.into<pdf::Scalar>().value()));
				space = Scalar();
			}

			m_words[i].first.render(cur_text, space);

			current_curs = m_words[i].second;
			current_curs.pos_on_page.x() += m_words[i].first.size().x();
		}
		pages[m_words.back().second.page_num]->addObject(cur_text);
	}
}
