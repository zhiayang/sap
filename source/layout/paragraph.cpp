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
	auto Paragraph::createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style) const
	    -> LayoutResult
	{
		cursor = cursor.newLine(0);
		auto _ = cs->evaluator().pushBlockContext(cursor, this);

		const_cast<Paragraph*>(this)->evaluateScripts(cs);
		const_cast<Paragraph*>(this)->processWordSeparators();

		if(m_contents.empty())
			return { std::move(cursor), std::nullopt };

		auto style = parent_style->extendWith(this->style());

		std::vector<std::unique_ptr<layout::Line>> layout_lines {};
		auto para_pos = cursor.position();
		Size2d para_size { 0, 0 };

		auto lines = layout::linebreak::breakLines(cursor, style, m_contents, cursor.widthAtCursor());

		size_t current_idx = 0;

		for(auto line_it = lines.begin(); line_it != lines.end(); ++line_it)
		{
			auto& broken_line = *line_it;

			auto words_begin = m_contents.begin() + (ssize_t) current_idx;
			auto words_end = m_contents.begin() + (ssize_t) current_idx + (ssize_t) broken_line.numParts();

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

			auto [new_cursor, layout_line] = layout::Line::fromInlineObjects(cs, //
			    cursor, broken_line, style,                                      //
			    std::span(words_begin, words_end),
			    /* is_last_line: */ line_it + 1 == lines.end());

			cursor = std::move(new_cursor);

			para_size.x() = std::max(para_size.x(), layout_line->layoutSize().x());
			para_size.y() += layout_line->layoutSize().y();

			layout_lines.push_back(std::move(layout_line));

			current_idx += broken_line.numParts();
		}

		return {
			cursor,
			std::unique_ptr<layout::Paragraph>(new layout::Paragraph(para_pos, para_size, std::move(layout_lines))),
		};
	}
}
