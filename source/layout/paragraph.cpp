// paragraph.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <variant> // for variant, visit, holds_alternative

#include "util.h"     // for dynamic_pointer_cast, overloaded
#include "dijkstra.h" // for dijkstra_shortest_path

#include "pdf/font.h"  // for Font
#include "pdf/page.h"  // for Page
#include "pdf/text.h"  // for Text
#include "pdf/units.h" // for pdf_typographic_unit_1d, Scalar, Positi...

#include "sap/style.h"    // for Style, Stylable
#include "sap/units.h"    // for Scalar
#include "sap/fontset.h"  // for FontSet
#include "sap/document.h" // for Word, Cursor, Paragraph, PositionedWord

#include "interp/tree.h"     // for Separator, Paragraph, Separator::SPACE
#include "interp/basedefs.h" // for InlineObject

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


	using WordOrSep = std::variant<Word, Separator>;
	using WordVec = std::vector<WordOrSep>;
	using WordVecIter = WordVec::const_iterator;

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
		Scalar line_height {};
		Scalar line_width_excluding_last_word {};
		std::optional<Separator> last_sep {};
		std::u32string last_word {};
		size_t num_spaces = 0;
		const Style* style = Style::empty();
		WordVec m_words;
		std::vector<Scalar> m_word_widths;
		Scalar m_total_word_width;
		Scalar m_total_space_width;

		// Helper functions
		static Size2d calculateWordSize(zst::wstr_view text, const Style* style)
		{
			return style->font()->getWordSize(text, style->font_size().into<pdf::Scalar>()).into<Size2d>();
		}

	public:
		// Getters
		Scalar width()
		{
			auto last_word_style = parent_style->extend(style);
			if(last_sep)
			{
				last_word += last_sep->endOfLine().sv();
				auto ret = line_width_excluding_last_word + calculateWordSize(last_word, last_word_style).x();
				last_word.substr(0, last_word.size() - last_sep->endOfLine().size());
				return ret;
			}
			else
			{
				auto ret = line_width_excluding_last_word + calculateWordSize(last_word, last_word_style).x();
				return ret;
			}
		}

		const std::vector<Scalar>& wordWidths() const { return m_word_widths; }
		Scalar totalWordWidth() const { return m_total_word_width; }
		Scalar totalSpaceWidth() const { return m_total_space_width; }

		size_t numSpaces() const { return num_spaces; }
		Cursor lineCursor() const { return layout->newLineFrom(prev_cursor, line_height); }
		const WordVec& words() const { return m_words; }

		// Modifiers
		void add(Word w)
		{
			auto prev_word_style = parent_style->extend(style);
			auto word_style = parent_style->extend(w.style());
			line_height = std::max(line_height,
			    calculateWordSize(last_word, word_style).y() * word_style->line_spacing().value());
			if(last_sep)
			{
				if(last_sep->kind == tree::Separator::SPACE)
				{
					line_width_excluding_last_word += calculateWordSize(last_word, prev_word_style).x();
					last_word.clear();

					Scalar space_width = std::max( //
					    calculateWordSize(last_sep->middleOfLine(), prev_word_style).x(),
					    calculateWordSize(last_sep->middleOfLine(), word_style).x());
					line_width_excluding_last_word += space_width;
					m_total_space_width += space_width;
					m_word_widths.push_back(space_width);
					m_words.emplace_back(*last_sep);
					num_spaces++;
				}
				else
				{
					sap::internal_error("support other seps {}", last_sep->kind);
				}
				last_sep.reset();

				style = w.style();
				last_word = w.text().sv();
				Scalar word_width = calculateWordSize(last_word, word_style).x();
				m_total_word_width += word_width;
				m_word_widths.push_back(word_width);
				m_words.emplace_back(w);
			}
			else if(style != nullptr && style != w.style() && *style != *w.style())
			{
				line_width_excluding_last_word += calculateWordSize(last_word, prev_word_style).x();
				last_word.clear();
				style = w.style();
				last_word = w.text().sv();
				Scalar word_width = calculateWordSize(last_word, word_style).x();
				m_total_word_width += word_width;
				m_word_widths.push_back(word_width);
				m_words.emplace_back(w);
			}
			else
			{
				style = w.style();
				Scalar prev_word_width = calculateWordSize(last_word, word_style).x();
				last_word += w.text().sv();
				Scalar new_word_width = calculateWordSize(last_word, word_style).x();
				m_total_word_width += new_word_width - prev_word_width;
				m_word_widths.push_back(new_word_width - prev_word_width);
				m_words.emplace_back(w);
			}
		}
		void add(Separator sep)
		{
			if(sep.kind == tree::Separator::SPACE)
			{
				last_sep = sep;
			}
			else
			{
				sap::internal_error("support other seps {}", sep.kind);
			}
		}

	private:
		Line() = default;
	};

	static std::vector<Line> break_lines(RectPageLayout* layout, Cursor cursor, const Style* parent_style, const WordVec& words,
	    Scalar preferred_line_length)
	{
		struct LineBreakNode
		{
			const WordVec* words;
			RectPageLayout* layout;
			const Style* parent_style;
			Scalar preferred_line_length;
			WordVecIter broken_until;
			WordVecIter end;
			Line line;

			using Distance = double;

			static LineBreakNode make_end(WordVecIter end)
			{
				return LineBreakNode {
					.words = nullptr,
					.layout = nullptr,
					.parent_style = nullptr,
					.preferred_line_length = 0,
					.broken_until = end,
					.end = end,
					.line = Line::invalid_line(),
				};
			}


			LineBreakNode make_neighbour(WordVecIter neighbour_broken_until, Line neighbour_line) const
			{
				return LineBreakNode {
					.words = words,
					.layout = layout,
					.parent_style = parent_style,
					.preferred_line_length = preferred_line_length,
					.broken_until = neighbour_broken_until,
					.end = end,
					.line = neighbour_line,
				};
			}

			std::vector<std::pair<LineBreakNode, Distance>> neighbours()
			{
				auto neighbour_broken_until = broken_until;
				Line neighbour_line(layout, parent_style, line.lineCursor());
				std::vector<std::pair<LineBreakNode, Distance>> ret;
				while(true)
				{
					if(neighbour_broken_until == end)
					{
						// last line cost calculation has 0 cost
						Distance cost = 0;
						ret.emplace_back(this->make_neighbour(neighbour_broken_until, neighbour_line), cost);
						return ret;
					}


					// Hold off on incrementing neighbour_broken_until
					// since the broken_until iterator needs to point *at* the separator.
					// TODO: make it not ugly lol
					auto wordorsep = *neighbour_broken_until++;
					std::visit(
					    [&](auto w) {
						    neighbour_line.add(w);
					    },
					    wordorsep);
					// TODO: allow shrinking of spaces by allowing lines to go past the end of the preferred_line_length
					// by 10% of the space width * num_spaces
					if(neighbour_line.width() >= preferred_line_length)
					{
						if(ret.empty())
						{
							sap::warn("line breaker", "ovErFUll \\hBOx, badNesS 10001!100!");
							ret.emplace_back(this->make_neighbour(neighbour_broken_until, neighbour_line), 10000);
						}
						return ret;
					}
					if(std::holds_alternative<Separator>(wordorsep))
					{
						Distance cost = 0;
						// If there are no spaces we pretend there's half a space,
						// so the cost is twice as high as having 1 space
						double extra_space_size = (preferred_line_length - line.width()).mm()
						                        / (neighbour_line.numSpaces() ? (double) neighbour_line.numSpaces() : 0.5);
						cost += extra_space_size * extra_space_size;
						// For now there's no other cost calculation haha
						ret.emplace_back(this->make_neighbour(neighbour_broken_until, neighbour_line), cost);
					}
				}
			}

			size_t hash() const { return static_cast<size_t>(broken_until - words->begin()); }
			bool operator==(LineBreakNode other) const { return broken_until == other.broken_until; }
		};

		auto path = util::dijkstra_shortest_path(
		    LineBreakNode {
		        .words = &words,
		        .layout = layout,
		        .parent_style = parent_style,
		        .preferred_line_length = preferred_line_length,
		        .broken_until = words.begin(),
		        .end = words.end(),
		        .line = Line(layout, parent_style, cursor),
		    },
		    LineBreakNode::make_end(words.end()));
		std::vector<Line> ret;
		for(const LineBreakNode& node : path)
		{
			ret.push_back(node.line);
		}
		return ret;
	}

	std::pair<std::optional<const tree::Paragraph*>, Cursor> Paragraph::layout(interp::Interpreter* cs, RectPageLayout* layout,
	    Cursor cursor, const Style* parent_style, const tree::Paragraph* treepara)
	{
		cursor = layout->newLineFrom(cursor, 0);

		using WordSepVec = std::vector<std::variant<Word, Separator>>;
		WordSepVec words_and_seps;

		for(auto& wordorsep : treepara->contents())
		{
			auto style = parent_style->extend(wordorsep->style());
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

		// Break lines
		std::vector<Line> lines = break_lines(layout, cursor, parent_style, words_and_seps, layout->getWidthAt(cursor));

		// Justify and add words to region
		for(auto it = lines.begin(); it != lines.end(); ++it)
		{
			auto& line = *it;

			cursor = line.lineCursor();
			auto desired_space_width = layout->getWidthAt(cursor) - line.totalWordWidth();
			auto total_space_width = line.totalSpaceWidth();

			if(it + 1 == lines.end())
				desired_space_width = total_space_width;
			double space_width_factor = desired_space_width / total_space_width;

			const Style* prev_word_style;

			for(size_t i = 0; i < line.words().size(); ++i)
			{
				auto word_width = line.wordWidths()[i];
				auto& wordorsep = line.words()[i];
				auto word_visitor = [&](const Word& word) {
					Cursor new_cursor = layout->moveRightFrom(cursor, word_width);
					para->m_words.push_back(PositionedWord {
					    word,
					    word.style()->font(),
					    word.style()->font_size().into<dim::units::pdf_typographic_unit_1d>(),
					    cursor,
					    new_cursor,
					});
					cursor = new_cursor;
					prev_word_style = word.style();
				};

				auto sep_visitor = [&](const Separator& sep) {
					Cursor end_of_space_cursor = layout->moveRightFrom(cursor, word_width);
					para->m_words.push_back(PositionedWord {
					    Word(U" ", prev_word_style),
					    prev_word_style->font(),
					    prev_word_style->font_size().into<dim::units::pdf_typographic_unit_1d>(),
					    cursor,
					    end_of_space_cursor,
					});
					Cursor new_cursor = layout->moveRightFrom(cursor, word_width * space_width_factor);
					cursor = new_cursor;
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

		Cursor current_curs = m_words.front().start;
		pdf::Text* cur_text = util::make<pdf::Text>();
		cur_text->moveAbs(pages[m_words.front().start.page_num]->convertVector2(
		    m_words.front().start.pos_on_page.into<pdf::Position2d_YDown>()));

		for(size_t i = 0; i < m_words.size(); i++)
		{
			cur_text->setFont(m_words[i].font, m_words[i].font_size);
			/*
			    not sure if this is legit, but words basically don't actually use their own `m_position` when
			    rendering; we just pass the words to the PDF, and the viewer uses the font metrics to adjust.

			    we do use the computed position when advancing to the next line, to account for line spacing
			    differences if there's more than one font-size on a line.
			*/
			auto space = m_words[i].start.pos_on_page.x() - current_curs.pos_on_page.x();

			if(current_curs.page_num != m_words[i].start.page_num)
			{
				pages[m_words[i - 1].start.page_num]->addObject(cur_text);
				cur_text = util::make<pdf::Text>();
				cur_text->moveAbs(
				    pages[m_words[i].start.page_num]->convertVector2(m_words[i].start.pos_on_page.into<pdf::Position2d_YDown>()));
				space = 0;
			}
			else if(current_curs.pos_on_page.y() != m_words[i].start.pos_on_page.y())
			{
				auto skip = m_words[i].start.pos_on_page.y() - current_curs.pos_on_page.y();
				cur_text->nextLine(pdf::Offset2d(0, -1.0 * skip.into<pdf::Scalar>().value()));
				space = 0;
			}
			else if(m_words[i].start.pos_on_page.x() != current_curs.pos_on_page.x())
			{
				// Make sure pdf cursor agrees with our cursor
				cur_text->offset(pdf::Font::convertPDFScalarToTextSpaceForFontSize(
				    (m_words[i].start.pos_on_page.x() - current_curs.pos_on_page.x()).into<pdf::Scalar>(), m_words[i].font_size));
			}

			m_words[i].word.render(cur_text, space);

			current_curs = m_words[i].end;
		}
		pages[m_words.back().start.page_num]->addObject(cur_text);
	}
}
