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
	Paragraph::Paragraph(const Style* style,
		LayoutSize size,
		std::vector<std::unique_ptr<Line>> lines,
		std::vector<std::unique_ptr<tree::InlineObject>> para_inline_objs)
		: LayoutObject(style, size)
		, m_lines(std::move(lines))
		, m_para_inline_objs(std::move(para_inline_objs))
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

		bool first = true;
		for(auto& line : m_lines)
		{
			if(not first)
				cursor = cursor.newLine(line->layoutSize().ascent);

			first = false;
			cursor = line->positionChildren(cursor);
		}

		return cursor;
	}
}


namespace sap::tree
{
	auto Paragraph::createLayoutObject(interp::Interpreter* cs, const Style* parent_style, Size2d available_space) const
		-> ErrorOr<LayoutResult>
	{
		auto _ = cs->evaluator().pushBlockContext(this);

		auto para_objects = TRY(this->evaluate_scripts(cs));
		para_objects = TRY(this->processWordSeparators(std::move(para_objects)));

		if(para_objects.empty())
			return Ok(LayoutResult::empty());

		auto style = parent_style->extendWith(this->style());

		std::vector<std::unique_ptr<layout::Line>> layout_lines {};
		LayoutSize para_size {};

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

		auto broken_lines = layout::linebreak::breakLines(style, para_objects, available_space.x());

		for(auto line_it = broken_lines.begin(); line_it != broken_lines.end(); ++line_it)
		{
			auto& broken_line = *line_it;

			auto words_begin = para_objects.begin() + (ssize_t) current_idx;
			auto words_end = para_objects.begin() + (ssize_t) current_idx + (ssize_t) broken_line.numParts();

			add_one_line(words_begin, words_end, style);

			current_idx += broken_line.numParts();
		}

		for(size_t i = 0; i < the_lines.size(); i++)
		{
			bool is_last_line = (i + 1 == the_lines.size());
			bool is_first_line = (i == 0);

			auto& [metrics, word_span] = the_lines[i];

			auto layout_line = layout::Line::fromInlineObjects(cs, style, word_span, metrics, available_space,
				is_first_line, is_last_line);

			auto line_size = layout_line->layoutSize();

			para_size.width = std::max(para_size.width, line_size.width);
			if(is_first_line)
			{
				para_size.ascent += line_size.ascent;
				para_size.descent += line_size.descent;
			}
			else
			{
				para_size.descent += line_size.total_height();
			}

			layout_lines.push_back(std::move(layout_line));
		}

		auto layout_para = std::unique_ptr<layout::Paragraph>(new layout::Paragraph(style, para_size,
			std::move(layout_lines), std::move(para_objects)));

		m_generated_layout_object = layout_para.get();
		return Ok(LayoutResult::make(std::move(layout_para)));
	}
}
