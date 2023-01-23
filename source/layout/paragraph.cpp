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

#include "interp/interp.h"
#include "interp/basedefs.h" // for InlineObject
#include "interp/evaluator.h"

#include "layout/base.h"      // for Cursor, Size2d, RectPageLayout, Position
#include "layout/line.h"      // for Line, breakLines
#include "layout/word.h"      // for Separator, Word
#include "layout/paragraph.h" // for Paragraph, PositionedWord
#include "layout/linebreak.h" //

namespace sap::layout
{
	std::unique_ptr<Paragraph> Paragraph::fromLines(interp::Interpreter* cs, //
	    PageCursor cursor,
	    std::vector<std::unique_ptr<Line>> lines)
	{
		auto widest_line = std::max_element(lines.begin(), lines.end(), [](auto& a, auto& b) {
			return a->layoutSize().x() < b->layoutSize().x();
		});

		auto line_width = (*widest_line).get()->layoutSize().x();
		auto total_height = std::accumulate(lines.begin(), lines.end(), Length(0), [](Length a, const auto& b) {
			return a + b->layoutSize().y();
		});

		return std::unique_ptr<Paragraph>(new Paragraph(cursor.position(), { line_width, total_height }, std::move(lines)));
	}

	Paragraph::Paragraph(RelativePos pos, Size2d size, std::vector<std::unique_ptr<Line>> lines)
	    : LayoutObject(pos, size)
	    , m_lines(std::move(lines))
	{
	}


	void Paragraph::render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		for(auto& line : m_lines)
			line->render(layout, pages);
	}
}


namespace sap::tree
{
	Size2d Paragraph::size(const layout::PageCursor& cursor) const
	{
		return { cursor.widthAtCursor(), 0 };
	}

	auto Paragraph::createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style) const
	    -> LayoutResult
	{
		std::vector<std::unique_ptr<InlineObject>> para_objects {};

		auto _ = cs->evaluator().pushBlockContext(cursor, this,
		    { .add_inline_object = [&para_objects](auto obj) -> ErrorOr<void> {
			    para_objects.push_back(std::move(obj));
			    return Ok();
		    } });


		const_cast<Paragraph*>(this)->evaluate_scripts(cs, para_objects);
		const_cast<Paragraph*>(this)->processWordSeparators();

		if(m_contents.empty())
			return LayoutResult::make(cursor);

		auto style = parent_style->extendWith(this->style());

		std::vector<std::unique_ptr<layout::Line>> layout_lines {};
		Size2d para_size { 0, 0 };

		size_t current_idx = 0;

		// precompute line metrics (and the line bounds)
		using WordSpan = std::span<const std::unique_ptr<tree::InlineObject>>;
		std::vector<std::tuple<layout::LineMetrics, WordSpan>> the_lines {};

		auto add_one_line = [&the_lines](auto words_begin, auto words_end, const Style* style) {
			if(dynamic_cast<const tree::Separator*>((*words_begin).get()) != nullptr)
			{
				sap::internal_error(
				    "line starting with non-Word found, "
				    "either line breaking algo is broken "
				    "or we had multiple separators in a row");
			}

			// Ignore space at end of line
			const auto& last_word = *(words_end - 1);
			if(auto sep = dynamic_cast<const tree::Separator*>(last_word.get()); sep && sep->isSpace())
				--words_end;

			auto words_span = std::span(words_begin, words_end);
			the_lines.push_back({
			    layout::computeLineMetrics(words_span, style),
			    words_span,
			});
		};


		if(not m_single_line_mode)
		{
			auto broken_lines = layout::linebreak::breakLines(cursor, style, m_contents, cursor.widthAtCursor());
			for(auto line_it = broken_lines.begin(); line_it != broken_lines.end(); ++line_it)
			{
				auto& broken_line = *line_it;

				auto words_begin = m_contents.begin() + (ssize_t) current_idx;
				auto words_end = m_contents.begin() + (ssize_t) current_idx + (ssize_t) broken_line.numParts();

				add_one_line(words_begin, words_end, style);

				current_idx += broken_line.numParts();
			}
		}
		else
		{
			add_one_line(m_contents.begin(), m_contents.end(), style);
		}



		for(size_t i = 0; i < the_lines.size(); i++)
		{
			bool is_last_line = i + 1 == the_lines.size();
			auto& [metrics, word_span] = the_lines[i];

			/*
			    Line typesets its words assuming the cursor is at the baseline;
			    we need to reserve vertical space to account for that.

			    however, if we are in "single line mode", then we typeset ourselves at the
			    baseline instead.
			*/
			if(i > 0 || not m_single_line_mode)
				cursor = cursor.newLine(metrics.ascent_height);


			auto [new_cursor, layout_line] = layout::Line::fromInlineObjects(cs, //
			    cursor, style, word_span, metrics, /* is_first: */ i == 0, is_last_line);

			cursor = std::move(new_cursor);
			cursor = cursor.carriageReturn();

			para_size.x() = std::max(para_size.x(), layout_line->layoutSize().x());
			para_size.y() += layout_line->layoutSize().y();

			layout_lines.push_back(std::move(layout_line));
		}

		auto para_pos2 = layout_lines.front()->layoutPosition();
		auto layout_para = std::unique_ptr<layout::Paragraph>(new layout::Paragraph(para_pos2, para_size,
		    std::move(layout_lines)));

		return LayoutResult::make(cursor, std::move(layout_para));
	}
}
