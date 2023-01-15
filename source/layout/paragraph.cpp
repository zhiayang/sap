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
	LineCursor Paragraph::fromTree(interp::Interpreter* cs, LayoutBase* layout, LineCursor cursor, const Style* parent_style,
	    const tree::DocumentObject* doc_obj)
	{
		auto treepara = static_cast<const tree::Paragraph*>(doc_obj);

		cursor = cursor.newLine(0);

		std::vector<std::unique_ptr<Line>> layout_lines {};
		auto para_pos = cursor.position();
		Size2d para_size { 0, 0 };

		auto& contents = treepara->contents();
		auto lines = linebreak::breakLines(layout, cursor, parent_style, contents, cursor.widthAtCursor());

		size_t current_idx = 0;

		for(auto line_it = lines.begin(); line_it != lines.end(); ++line_it)
		{
			auto& broken_line = *line_it;

			auto words_begin = contents.begin() + (ssize_t) current_idx;
			auto words_end = contents.begin() + (ssize_t) current_idx + (ssize_t) broken_line.numParts();

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

			// zpr::println("[{02}] line height = {}", line_it - lines.begin(), broken_line.lineHeight());
			cursor = cursor.newLine(broken_line.lineHeight());

			auto layout_line = Line::fromInlineObjects(cursor, broken_line, parent_style, std::span(words_begin, words_end));
			para_size.x() = std::max(para_size.x(), layout_line->layoutSize().x());
			para_size.y() += layout_line->layoutSize().y();

			layout_lines.push_back(std::move(layout_line));

			current_idx += broken_line.numParts();
		}



		layout->addObject(std::unique_ptr<Paragraph>(new Paragraph(para_pos, para_size, std::move(layout_lines))));
		cursor = cursor.newLine(parent_style->paragraph_spacing());
		return cursor;
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
