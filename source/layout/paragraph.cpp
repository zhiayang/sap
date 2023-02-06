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
	Paragraph::Paragraph(const Style* style, Size2d size, std::vector<std::unique_ptr<Line>> lines)
		: LayoutObject(style, size)
		, m_lines(std::move(lines))
	{
	}

	void Paragraph::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		for(auto& line : m_lines)
			line->render(layout, pages);
	}

	layout::PageCursor Paragraph::positionChildren(layout::PageCursor cursor)
	{
		assert(not m_lines.empty());
		this->positionRelatively(cursor.position());

		// this is necessary because lines are typeset with their "position" being
		// the text baseline; we want the paragraph to be positioned wrt. the top-left corner,
		// so we move down by the ascent of the first line to achieve this offset.
		cursor = cursor.newLine(m_lines[0]->metrics().ascent_height);

		for(auto& line : m_lines)
			cursor = line->positionChildren(cursor);

		return cursor;
	}
}


namespace sap::tree
{
	auto Paragraph::createLayoutObject(interp::Interpreter* cs, const Style* parent_style, Size2d available_space) const
		-> ErrorOr<LayoutResult>
	{
		auto _ = cs->evaluator().pushBlockContext(this);

		std::vector<std::unique_ptr<InlineObject>> para_objects {};
		const_cast<Paragraph*>(this)->evaluate_scripts(cs, para_objects);
		const_cast<Paragraph*>(this)->processWordSeparators();

		if(m_contents.empty())
			return Ok(LayoutResult::empty());

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

		auto broken_lines = layout::linebreak::breakLines(style, m_contents, available_space.x());

		for(auto line_it = broken_lines.begin(); line_it != broken_lines.end(); ++line_it)
		{
			auto& broken_line = *line_it;

			auto words_begin = m_contents.begin() + (ssize_t) current_idx;
			auto words_end = m_contents.begin() + (ssize_t) current_idx + (ssize_t) broken_line.numParts();

			add_one_line(words_begin, words_end, style);

			current_idx += broken_line.numParts();
		}

		for(size_t i = 0; i < the_lines.size(); i++)
		{
			bool is_last_line = i + 1 == the_lines.size();
			auto& [metrics, word_span] = the_lines[i];

			auto layout_line = layout::Line::fromInlineObjects(cs, style, word_span, metrics, available_space,
				/* is_first: */ i == 0, is_last_line);

			para_size.x() = std::max(para_size.x(), layout_line->layoutSize().x());
			para_size.y() += layout_line->layoutSize().y();

			layout_lines.push_back(std::move(layout_line));
		}

		auto layout_para = std::unique_ptr<layout::Paragraph>(new layout::Paragraph(style, para_size, std::move(layout_lines)));

		return Ok(LayoutResult::make(std::move(layout_para)));
	}
}
