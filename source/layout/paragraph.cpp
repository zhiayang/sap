// paragraph.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <variant> // for variant, get, holds_alternative, visit

#include "util.h" // for overloaded

#include "pdf/font.h"  // for Font
#include "pdf/page.h"  // for Page
#include "pdf/text.h"  // for Text
#include "pdf/units.h" // for PdfScalar, Position2d_YDown, Offset2d

#include "sap/style.h" // for Style
#include "sap/units.h" // for Length

#include "interp/tree.h"     // for Separator, Paragraph, Text, Separator:...
#include "interp/value.h"    // for Interpreter
#include "interp/basedefs.h" // for InlineObject

#include "layout/base.h"      // for Cursor, Size2d, RectPageLayout, Position
#include "layout/line.h"      // for Line, breakLines
#include "layout/word.h"      // for Separator, Word
#include "layout/paragraph.h" // for Paragraph, PositionedWord

namespace sap::layout
{
	static Size2d calculateWordSize(zst::wstr_view text, const Style* style)
	{
		return style->font()->getWordSize(text, style->font_size().into<pdf::PdfScalar>()).into<Size2d>();
	}

	struct LineMetrics
	{
		sap::Length total_space_width;
		sap::Length total_word_width;
		std::vector<sap::Length> word_widths;
		sap::Length line_height;

		template <typename It>
		static LineMetrics computeLineMetrics(It words_begin, It words_end, const Style* parent_style)
		{
			LineMetrics ret;

			sap::Length word_chunk_width;
			const Style* word_chunk_style;
			std::u32string word_chunk_text;
			auto word_chunk_it = words_begin;

			for(auto it = words_begin; it != words_end; ++it)
			{
				if(auto word = std::get_if<Word>(&*it); word)
				{
					auto style = parent_style->extendWith(word->style());
					if(it != word_chunk_it && word_chunk_style != style)
					{
						ret.total_word_width += word_chunk_width;
						word_chunk_width = 0;
						word_chunk_style = style;
						word_chunk_text.clear();
						word_chunk_it = it;
					}
					word_chunk_text += word->text().sv();
					auto chunk_size = calculateWordSize(word_chunk_text, style);
					ret.word_widths.push_back(chunk_size.x() - word_chunk_width);
					word_chunk_width = chunk_size.x();

					ret.line_height = std::max(ret.line_height, chunk_size.y());
				}
				else if(auto sep = std::get_if<Separator>(&*it); sep)
				{
					const auto& prev_word = std::get<Word>(*(it - 1));
					const auto& next_word = std::get<Word>(*(it + 1));
					ret.total_word_width += word_chunk_width;

					word_chunk_width = 0;
					word_chunk_style = nullptr;
					word_chunk_text.clear();
					word_chunk_it = it + 1;

					auto sep_char = (it + 1 == words_end ? sep->endOfLine() : sep->middleOfLine());
					auto sep_width = std::max(calculateWordSize(sep_char, prev_word.style()).x(),
					    calculateWordSize(sep_char, next_word.style()).x());

					if(sep->kind == tree::Separator::SPACE)
						ret.total_space_width += sep_width;
					else
						ret.total_word_width += sep_width;

					ret.word_widths.push_back(sep_width);
				}
			}

			ret.total_word_width += word_chunk_width;
			return ret;
		}
	};


	std::pair<std::optional<const tree::Paragraph*>, Cursor> Paragraph::layout(interp::Interpreter* cs, RectPageLayout* layout,
	    Cursor cursor, const Style* parent_style, const tree::Paragraph* treepara)
	{
		cursor = layout->newLineFrom(cursor, 0);

		using WordSepVec = std::vector<std::variant<Word, Separator>>;
		WordSepVec words_and_seps;

		for(auto& wordorsep : treepara->contents())
		{
			auto style = parent_style->extendWith(wordorsep->style());
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
		for(auto line_it = lines.begin(); line_it != lines.end(); ++line_it)
		{
			auto& line1 = *line_it;

			auto words_begin = words_and_seps.begin() + (ssize_t) current_idx;
			auto words_end = words_and_seps.begin() + (ssize_t) current_idx + (ssize_t) line1.numParts();

			if(not std::holds_alternative<Word>(*words_begin))
			{
				sap::internal_error(
				    "line starting with non-Word found, "
				    "either line breaking algo is broken "
				    "or we had multiple separators in a row");
			}

			// Ignore space at end of line
			const auto& last_word = *(words_end - 1);
			if(auto sep = std::get_if<Separator>(&last_word); sep && sep->kind == tree::Separator::SPACE)
				--words_end;

			auto line_metrics = LineMetrics::computeLineMetrics(words_begin, words_end, parent_style);

			cursor = layout->newLineFrom(cursor, line_metrics.line_height);
			auto desired_space_width = layout->getWidthAt(cursor) - line_metrics.total_word_width;
			double space_width_factor = desired_space_width / line_metrics.total_space_width;

			if(line_it + 1 == lines.end())
				space_width_factor = std::min(prev_space_width_factor, space_width_factor);

			const Style* prev_word_style = Style::empty();
			size_t num_words = (size_t) (words_end - words_begin);
			for(size_t i = 0; i < num_words; i++)
			{
				auto word_width = line_metrics.word_widths[i];

				auto word_visitor = [&](const Word& word) {
					auto new_cursor = layout->moveRightFrom(cursor, word_width);
					para->m_words.push_back(PositionedWord {
					    .word = word,
					    .font = word.style()->font(),
					    .font_size = word.style()->font_size().into<pdf::PdfScalar>(),
					    .start = cursor,
					    .end = new_cursor,
					});

					cursor = new_cursor;
					prev_word_style = word.style();
				};

				auto sep_visitor = [&](const Separator& sep) {
					auto end_of_space_cursor = layout->moveRightFrom(cursor, word_width);
					para->m_words.push_back(PositionedWord {
					    .word = Word(i + 1 == num_words ? sep.endOfLine() : sep.middleOfLine(), prev_word_style),
					    .font = prev_word_style->font(),
					    .font_size = prev_word_style->font_size().into<pdf::PdfScalar>(),
					    .start = cursor,
					    .end = end_of_space_cursor,
					});

					auto new_cursor = layout->moveRightFrom(cursor, word_width * space_width_factor);
					cursor = new_cursor;
				};

				std::visit(util::overloaded { word_visitor, sep_visitor }, words_and_seps[i + current_idx]);
			}

			current_idx += line1.numParts();
			prev_space_width_factor = space_width_factor;
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
				cur_text->nextLine(pdf::Offset2d(0, -1.0 * skip.into<pdf::PdfScalar>().value()));
				space = 0;
			}
			else if(m_words[i].start.pos_on_page.x() != current_curs.pos_on_page.x())
			{
				// Make sure pdf cursor agrees with our cursor
				cur_text->offset(pdf::PdfFont::convertPDFScalarToTextSpaceForFontSize(
				    (m_words[i].start.pos_on_page.x() - current_curs.pos_on_page.x()).into<pdf::PdfScalar>(),
				    m_words[i].font_size));
			}

			m_words[i].word.render(cur_text, space);

			current_curs = m_words[i].end;
		}
		pages[m_words.back().start.page_num]->addObject(cur_text);
	}
}
