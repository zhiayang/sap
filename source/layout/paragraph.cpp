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

#include "interp/value.h"    // for Interpreter
#include "interp/basedefs.h" // for InlineObject

#include "layout/base.h"      // for Cursor, Size2d, RectPageLayout, Position
#include "layout/line.h"      // for Line, breakLines
#include "layout/word.h"      // for Separator, Word
#include "layout/paragraph.h" // for Paragraph, PositionedWord
#include "layout/linebreak.h" //

namespace sap::layout
{
	static Size2d calculate_word_size(zst::wstr_view text, const Style* style)
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

			struct WordChunk
			{
				Length width = 0;
				std::u32string text;
				const Style* style = nullptr;
			};

			WordChunk word_chunk {};
			auto word_chunk_it = words_begin;

			for(auto it = words_begin; it != words_end; ++it)
			{
				if(auto word = std::get_if<Word>(&*it); word)
				{
					auto style = parent_style->extendWith(word->style());
					if(it != word_chunk_it && word_chunk.style != style)
					{
						ret.total_word_width += word_chunk.width;

						word_chunk.width = 0;
						word_chunk.style = style;
						word_chunk.text.clear();
						word_chunk_it = it;
					}

					word_chunk.text += word->text().sv();

					auto chunk_size = calculate_word_size(word_chunk.text, style);
					ret.word_widths.push_back(chunk_size.x() - word_chunk.width);

					word_chunk.width = chunk_size.x();

					ret.line_height = std::max(ret.line_height, chunk_size.y() * style->line_spacing());
				}
				else if(auto sep = std::get_if<Separator>(&*it); sep)
				{
					const auto& prev_word = std::get<Word>(*(it - 1));
					const auto& next_word = std::get<Word>(*(it + 1));
					ret.total_word_width += word_chunk.width;

					word_chunk.width = 0;
					word_chunk.style = nullptr;
					word_chunk.text.clear();

					word_chunk_it = it + 1;

					auto sep_char = (it + 1 == words_end ? sep->endOfLine() : sep->middleOfLine());
					auto sep_width = std::max(calculate_word_size(sep_char, prev_word.style()).x(),
					    calculate_word_size(sep_char, next_word.style()).x());

					if(sep->isSpace())
						ret.total_space_width += sep_width;
					else
						ret.total_word_width += sep_width;

					ret.word_widths.push_back(sep_width);
				}
			}

			ret.total_word_width += word_chunk.width;
			return ret;
		}
	};


	LineCursor Paragraph::fromTree(interp::Interpreter* cs, LayoutBase* layout, LineCursor cursor, const Style* parent_style,
	    const tree::DocumentObject* doc_obj)
	{
		auto treepara = static_cast<const tree::Paragraph*>(doc_obj);

		using WordSepVec = std::vector<std::variant<Word, Separator>>;
		WordSepVec words_and_seps;

		for(auto& wordorsep : treepara->contents())
		{
			auto style = parent_style->extendWith(wordorsep->style());
			if(auto word = dynamic_cast<tree::Text*>(wordorsep.get()); word != nullptr)
				words_and_seps.push_back(Word(word->contents(), style));

			else if(auto sep = dynamic_cast<tree::Separator*>(wordorsep.get()); sep != nullptr)
				words_and_seps.push_back(Separator(sep->kind(), sep->style(), sep->hyphenationCost()));
		}

		auto para = std::make_unique<Paragraph>();
		cursor = cursor.newLine(0);

		para->m_layout_position = cursor.position();

		auto lines = linebreak::breakLines(layout, cursor, parent_style, words_and_seps, cursor.widthAtCursor());

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
			if(auto sep = std::get_if<Separator>(&last_word); sep && sep->isSpace())
				--words_end;

			auto line_metrics = LineMetrics::computeLineMetrics(words_begin, words_end, parent_style);

			cursor = cursor.newLine(line_metrics.line_height);
			auto desired_space_width = cursor.widthAtCursor() - line_metrics.total_word_width;
			double space_width_factor = desired_space_width / line_metrics.total_space_width;

			if(line_it + 1 == lines.end())
				space_width_factor = std::min(prev_space_width_factor, space_width_factor);

			const Style* prev_word_style = Style::empty();
			size_t num_words = (size_t) (words_end - words_begin);
			for(size_t i = 0; i < num_words; i++)
			{
				auto word_width = line_metrics.word_widths[i];

				auto word_visitor = [&](const Word& word) {
					auto new_cursor = cursor.moveRight(word_width);
					para->m_words.push_back(PositionedWord {
					    .word = word,
					    .font = word.style()->font(),
					    .font_size = word.style()->font_size().into<pdf::PdfScalar>(),
					    .start = cursor.position(),
					    .end = new_cursor.position(),
					});

					cursor = std::move(new_cursor);
					prev_word_style = word.style();
				};

				auto sep_visitor = [&](const Separator& sep) {
					auto end_of_space_cursor = cursor.moveRight(word_width);
					para->m_words.push_back(PositionedWord {
					    .word = Word(i + 1 == num_words ? sep.endOfLine() : sep.middleOfLine(), prev_word_style),
					    .font = prev_word_style->font(),
					    .font_size = prev_word_style->font_size().into<pdf::PdfScalar>(),
					    .start = cursor.position(),
					    .end = end_of_space_cursor.position(),
					});

					cursor = cursor.moveRight(word_width * space_width_factor);
				};

				std::visit(util::overloaded { word_visitor, sep_visitor }, words_and_seps[i + current_idx]);
			}

			para->m_layout_size.y() += line_metrics.line_height;
			para->m_layout_size.x() = std::max(para->m_layout_size.x(),
			    para->m_words.back().end.pos.x() - para->m_layout_position.pos.x());

			current_idx += line1.numParts();
			prev_space_width_factor = space_width_factor;
		}

		layout->addObject(std::move(para));
		cursor = cursor.newLine(parent_style->paragraph_spacing());
		return cursor;
	}


	void Paragraph::render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		if(m_words.empty())
			return;

		auto current_curs = layout->convertPosition(m_words.front().start);

		pdf::Text* cur_text = util::make<pdf::Text>();
		pages[current_curs.page_num]->addObject(cur_text);
		cur_text->moveAbs(pages[current_curs.page_num]->convertVector2(current_curs.pos.into<pdf::Position2d_YDown>()));

		for(size_t i = 0; i < m_words.size(); i++)
		{
			cur_text->setFont(m_words[i].font, m_words[i].font_size);

			auto word_pos = layout->convertPosition(m_words[i].start);
			auto space = (word_pos.pos - current_curs.pos).x();

			if(current_curs.page_num != word_pos.page_num)
			{
				cur_text = util::make<pdf::Text>();
				cur_text->moveAbs(pages[word_pos.page_num]->convertVector2(word_pos.pos.into<pdf::Position2d_YDown>()));
				pages[word_pos.page_num]->addObject(cur_text);

				space = 0;
			}
			else if(current_curs.pos.y() != word_pos.pos.y())
			{
				auto skip = word_pos.pos.y() - current_curs.pos.y();
				cur_text->nextLine(pdf::Offset2d(0, -1.0 * skip.into<pdf::PdfScalar>().value()));
				space = 0;
			}
			else if(word_pos.pos.x() != current_curs.pos.x())
			{
				// Make sure pdf cursor agrees with our cursor
				cur_text->offset(pdf::PdfFont::convertPDFScalarToTextSpaceForFontSize(
				    (word_pos.pos - current_curs.pos).x().into<pdf::PdfScalar>(), m_words[i].font_size));
			}

			m_words[i].word.render(cur_text, space);

			current_curs = layout->convertPosition(m_words[i].end);
		}
	}
}
