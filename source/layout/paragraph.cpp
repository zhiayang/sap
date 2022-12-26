// paragraph.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <numbers>
#include <variant> // for variant, visit

#include "util.h" // for dynamic_pointer_cast

#include "pdf/page.h"  // for Page
#include "pdf/text.h"  // for Text
#include "pdf/units.h" // for Position2d_YDown, Scalar, Offset2d

#include "sap/style.h"    // for Style
#include "sap/units.h"    // for Scalar
#include "sap/document.h" // for Word, Cursor, Paragraph, RectPageLayout
#include "dijkstra.h"

#include "zst.h"

#include "interp/tree.h" // for Paragraph, Text

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

		tree::Separator::SeparatorKind kind;

	private:
		char32_t m_end_of_line_char;
		char32_t m_middle_of_line_char;
	};

	static util::hashmap<std::u32string, Scalar> s_cache {};
	static Scalar get_word_width(zst::wstr_view string, const Style* style)
	{
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

	using WordOrSep = std::variant<Word, Separator>;
	using WordVec = std::vector<WordOrSep>;
	using WordVecIter = WordVec::const_iterator;

	[[maybe_unused]] static std::vector<WordVecIter> break_lines(const Style* parent_style, const WordVec& words,
	    Scalar preferred_line_length)
	{
		struct Line
		{
			WordVecIter begin;
			WordVecIter end;
			const Style* parent_style;

			Line(WordVecIter begin, WordVecIter end, const Style* parent_style)
			    : begin(begin)
			    , end(end)
			    , parent_style(parent_style)
			{
			}

			Scalar line_width_excluding_last_word {};
			std::optional<Separator> last_sep {};
			std::u32string last_word {};
			size_t num_spaces = 0;
			const Style* style = nullptr;

			void addWord()
			{
				WordOrSep w = *end++;
				std::visit(
				    util::overloaded {
				        [this](Word w) {
					        if(last_sep)
					        {
						        line_width_excluding_last_word += get_word_width(last_word, Style::combine(style, parent_style));
						        last_word.clear();
						        if(last_sep->kind == tree::Separator::SPACE)
						        {
							        line_width_excluding_last_word += std::
							            max(get_word_width(last_sep->middleOfLine(), Style::combine(style, parent_style)),
							                get_word_width(last_sep->middleOfLine(), Style::combine(w.style(), parent_style)));
							        num_spaces++;
						        }
						        else
						        {
							        line_width_excluding_last_word += get_word_width(last_sep->middleOfLine(), style);
						        }
						        last_sep.reset();
						        last_word = w.text().sv();
					        }
					        else if(style != nullptr && style != w.style() && *style != *w.style())
					        {
						        line_width_excluding_last_word += get_word_width(last_word, Style::combine(style, parent_style));
						        last_word.clear();
						        last_word = w.text().sv();
						        style = w.style();
					        }
					        else
					        {
						        style = w.style();
						        last_word += w.text().sv();
					        }
				        },
				        [this](Separator sep) {
					        last_sep = sep;
				        },
				    },
				    w);
			}

			Scalar calculateWidth()
			{
				if(last_sep)
				{
					last_word += last_sep->endOfLine().sv();
					auto ret = line_width_excluding_last_word + get_word_width(last_word, Style::combine(style, parent_style));
					last_word.substr(0, last_word.size() - last_sep->endOfLine().size());
					return ret;
				}
				else
				{
					auto ret = line_width_excluding_last_word + get_word_width(last_word, Style::combine(style, parent_style));
					return ret;
				}
			}
		};

		struct LineBreakNode
		{
			const WordVec* words;
			const Style* parent_style;
			Scalar preferred_line_length;
			WordVecIter broken_until;
			WordVecIter end;

			using Distance = double;

			LineBreakNode node_at_breakpoint(WordVecIter new_broken_until) const
			{
				return LineBreakNode { words, parent_style, preferred_line_length, new_broken_until, end };
			}

			std::vector<std::pair<LineBreakNode, Distance>> neighbours()
			{
				Line line(broken_until, broken_until, parent_style);
				std::vector<std::pair<LineBreakNode, Distance>> ret;
				while(true)
				{
					if(line.end == end)
					{
						// last line cost calculation has 0 cost
						Distance cost = 0;
						ret.emplace_back(this->node_at_breakpoint(line.end), cost);
						break;
					}
					line.addWord();
					// TODO: allow shrinking of spaces by allowing lines to go past the end of the preferred_line_length
					// by 10% of the space width * num_spaces
					if(line.calculateWidth() >= preferred_line_length)
					{
						break;
					}
					if(std::holds_alternative<Separator>(*line.end))
					{
						Distance cost = 0;
						// If there are no spaces we pretend there's half a space,
						// so the cost is twice as high as having 1 space
						double extra_space_size = (preferred_line_length - line.calculateWidth()).mm()
						                        / (line.num_spaces ? (double) line.num_spaces : 0.5);
						cost += extra_space_size * extra_space_size;
						// For now there's no other cost calculation haha
						ret.emplace_back(this->node_at_breakpoint(line.end), cost);
					}
				}
				if(ret.empty())
					sap::warn("line breaker", "ovErFUll \\hBOx, badNesS 10001!100!");
				ret.emplace_back(this->node_at_breakpoint(broken_until + 1), 10000);
				return ret;
			}

			size_t hash() const { return static_cast<size_t>(broken_until - words->begin()); }
			bool operator==(LineBreakNode other) const { return broken_until == other.broken_until; }
		};

		auto path = util::dijkstra_shortest_path(                                                      //
		    LineBreakNode { &words, parent_style, preferred_line_length, words.begin(), words.end() }, //
		    LineBreakNode { nullptr, parent_style, preferred_line_length, words.end(), words.end() });
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
			{
				words_and_seps.push_back(Word(word->contents(), style));
			}
			else if(auto sep = util::dynamic_pointer_cast<tree::Separator>(wordorsep); sep != nullptr)
			{
				words_and_seps.push_back(Separator(sep->kind(), sep->style()));
			}
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
			size_t num_spaces;
		};

		// Break lines
		size_t num_spaces = 0;
		std::vector<Line> lines;
		std::vector<WordVecIter> breakpoints = break_lines(parent_style, words_and_seps, layout->getWidthAt(cursor));
		auto breakit = breakpoints.begin();
		for(auto it = words_and_seps.begin(); it != words_and_seps.end(); ++it)
		{
			auto& wordorsep = *it;
			auto word_visitor = [&](const Word& word) {
				prev_space = word.spaceWidth();
				cur_line_spacing = std::max(cur_line_spacing, word.size().y() + word.style()->line_spacing());
				// TODO: line spacing calculation is wrong

				cur_line_length += word.size().x();
				cur_line.push_back(word);
			};

			auto sep_visitor = [&](const Separator& sep) {
				if(&*it == &**breakit)
				{
					// TODO: fix line length calculation
					++breakit;
					cursor = layout->newLineFrom(cursor, cur_line_spacing);
					lines.push_back(Line {
					    .words = std::move(cur_line),
					    .cursor = cursor,
					    .line_length = cur_line_length,
					    .num_spaces = num_spaces,
					});
					cur_line.clear();
					cur_line_length = 0;
					num_spaces = 0;

					cursor = layout->newLineFrom(cursor, 0);
				}
				else
				{
					// TODO: not all Separators are spaces
					num_spaces++;
					cur_line_length += prev_space;
					cur_line.push_back(sep);
				}
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
			auto num_spaces = line.num_spaces;
			auto extra_space_width = num_spaces == 0 ? 0 : total_extra_space_width / (double) num_spaces;

			if(it + 1 == lines.end())
				extra_space_width = 0;

			Scalar space_width;
			for(auto& wordorsep : line.words)
			{
				auto word_visitor = [&](const Word& word) {
					para->m_words.emplace_back(word, cursor);

					space_width = word.spaceWidth();
					cursor = layout->moveRightFrom(cursor, word.size().x());
				};

				auto sep_visitor = [&](const Separator& sep) {
					cursor = layout->moveRightFrom(cursor, space_width);
					cursor = layout->moveRightFrom(cursor, extra_space_width);
				};

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
