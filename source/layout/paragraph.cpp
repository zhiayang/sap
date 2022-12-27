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

#include "sap/style.h"   // for Style, Stylable
#include "sap/units.h"   // for Scalar
#include "sap/fontset.h" // for FontSet

#include "interp/tree.h"     // for Separator, Paragraph, Separator::SPACE
#include "interp/basedefs.h" // for InlineObject

#include "layout/word.h"
#include "layout/line.h"
#include "layout/paragraph.h"

namespace sap::layout
{
	struct Line2
	{
		// "Input"
		RectPageLayout* layout;
		const Style* parent_style;
		Cursor prev_cursor;

		// Sometimes we need to populate a line in a struct even though it's never read... yeah i'm sorry :(
		static Line2 invalid_line() { return Line2(); }

		Line2(RectPageLayout* layout, const Style* parent_style, Cursor prev_cursor)
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
		Scalar m_total_word_width;
		Scalar m_total_space_width;
		std::vector<Scalar> m_word_widths;

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
				last_word.erase(last_word.size() - last_sep->endOfLine().size());
				return ret;
			}
			else
			{
				return line_width_excluding_last_word + calculateWordSize(last_word, last_word_style).x();
			}
		}

		// const std::vector<Scalar>& wordWidths() const { return m_word_widths; }
		Scalar totalWordWidth() const { return m_total_word_width; }
		Scalar totalSpaceWidth() const { return m_total_space_width; }

		Cursor lineCursor() const { return layout->newLineFrom(prev_cursor, line_height); }
		size_t numParts() const { return m_word_widths.size(); }

		Scalar wordWidthForIndex(size_t idx) const { return m_word_widths[idx]; }

		// Modifiers
		void add(const Word& w)
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
			}
			else
			{
				style = w.style();
				Scalar prev_word_width = calculateWordSize(last_word, word_style).x();
				last_word += w.text().sv();
				Scalar new_word_width = calculateWordSize(last_word, word_style).x();
				m_total_word_width += new_word_width - prev_word_width;
				m_word_widths.push_back(new_word_width - prev_word_width);
			}
		}

		void add(Separator sep)
		{
			if(sep.kind == tree::Separator::SPACE)
				last_sep = sep;
			else
				sap::internal_error("support other seps {}", sep.kind);
		}

	private:
		Line2() = default;
	};


	std::pair<std::optional<const tree::Paragraph*>, Cursor> Paragraph::layout(interp::Interpreter* cs, RectPageLayout* layout,
	    Cursor cursor, const Style* parent_style, const tree::Paragraph* treepara)
	{
		cursor = layout->newLineFrom(cursor, 0);

		using WordSepVec = std::vector<std::variant<Word, Separator>>;
		WordSepVec words_and_seps;

		for(auto& wordorsep : treepara->contents())
		{
			auto style = parent_style->extend(wordorsep->style());
			if(auto word = dynamic_cast<tree::Text*>(wordorsep.get()); word != nullptr)
				words_and_seps.push_back(Word(word->contents(), style));

			else if(auto sep = dynamic_cast<tree::Separator*>(wordorsep.get()); sep != nullptr)
				words_and_seps.push_back(Separator(sep->kind(), sep->style()));
		}

		auto para = util::make<Paragraph>();
		auto lines = breakLines(layout, cursor, parent_style, words_and_seps, layout->getWidthAt(cursor));

		size_t current_idx = 0;

		// Justify and add words to region
		double prev_space_width_factor = 1;
		for(auto it = lines.begin(); it != lines.end(); ++it)
		{
			auto& line1 = *it;
			auto line2 = Line2(line1.layout, line1.parent_style, line1.prev_cursor);

			for(size_t i = 0; i < line1.numParts(); i++)
			{
				if(auto& ws = words_and_seps[current_idx + i]; std::holds_alternative<Word>(ws))
					line2.add(std::get<Word>(ws));
				else
					line2.add(std::get<Separator>(ws));
			}


			cursor = line2.lineCursor();
			auto desired_space_width = layout->getWidthAt(cursor) - line2.totalWordWidth();
			auto total_space_width = line2.totalSpaceWidth();

			double space_width_factor = desired_space_width / total_space_width;
			if(it + 1 == lines.end())
				space_width_factor = std::min(prev_space_width_factor, space_width_factor);

			const Style* prev_word_style = nullptr;


			for(size_t i = 0; i < line1.numParts(); i++)
			{
				auto word_width = line2.wordWidthForIndex(i);

				auto word_visitor = [&](const Word& word) {
					auto new_cursor = layout->moveRightFrom(cursor, word_width);
					para->m_words.push_back(PositionedWord {
					    .word = word,
					    .font = word.style()->font(),
					    .font_size = word.style()->font_size().into<pdf::Scalar>(),
					    .start = cursor,
					    .end = new_cursor,
					});

					cursor = new_cursor;
					prev_word_style = word.style();
				};

				auto sep_visitor = [&](const Separator& sep) {
					auto end_of_space_cursor = layout->moveRightFrom(cursor, word_width);
					para->m_words.push_back(PositionedWord {
					    .word = Word(U" ", prev_word_style),
					    .font = prev_word_style->font(),
					    .font_size = prev_word_style->font_size().into<pdf::Scalar>(),
					    .start = cursor,
					    .end = end_of_space_cursor,
					});

					auto new_cursor = layout->moveRightFrom(cursor, word_width * space_width_factor);
					cursor = new_cursor;
				};

				std::visit(util::overloaded { word_visitor, sep_visitor }, words_and_seps[i + current_idx]);
			}
<<<<<<< HEAD

			current_idx += line1.numParts();
=======
			prev_space_width_factor = space_width_factor;
>>>>>>> 384c23c (last line copy second last line's space width factor)
		}

		layout->addObject(para);
		return { std::nullopt, cursor };
	}


	void Paragraph::render(const RectPageLayout* layout, std::vector<pdf::Page*>& pages) const
	{
		if(m_words.empty())
			return;

		Cursor current_curs = m_words.front().start;
		pdf::Text* cur_text = util::make<pdf::Text>();
		cur_text->moveAbs(pages[m_words.front().start.page_num]->convertVector2(
		    m_words.front().start.pos_on_page.into<pdf::Position2d_YDown>()));

		for(size_t i = 0; i < m_words.size(); i++)
		{
			cur_text->setFont(m_words[i].font, m_words[i].font_size);

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
