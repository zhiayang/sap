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
#include "layout/container.h" //
#include "layout/paragraph.h" // for Paragraph, PositionedWord
#include "layout/linebreak.h" //

namespace sap::layout
{
	Paragraph::Paragraph(const Style& style,
	    LayoutSize size,
	    std::vector<std::unique_ptr<Line>> lines,
	    std::vector<zst::SharedPtr<tree::InlineObject>> para_inline_objs)
	    : LayoutObject(style, size), m_lines(std::move(lines)), m_para_inline_objs(std::move(para_inline_objs))
	{
	}

	void Paragraph::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		for(auto& line : m_lines)
			line->render(layout, pages);
	}

	layout::PageCursor Paragraph::compute_position_impl(layout::PageCursor cursor)
	{
		assert(not m_lines.empty());
		this->positionRelatively(cursor.position());

		using enum Alignment;
		using enum Container::Direction;

		if(m_lines.empty())
			return cursor;

		auto initial_pos = cursor.position().pos;

		bool is_first_child = true;

		size_t prev_child_bottom_page = 0;
		bool prev_child_was_phantom = false;

		for(auto& line : m_lines)
		{
			// preserve horizontal alignment of the words within the line
			cursor = cursor.carriageReturn();
			cursor = cursor.moveRight(initial_pos.x() - cursor.position().pos.x());

			auto horz_space = cursor.widthAtCursor();
			auto space_width = std::max(Length(0), m_layout_size.width - line->layoutSize().width);

			switch(m_style.horz_alignment())
			{
				case Left:
				case Justified: break;

				case Right: {
					cursor = cursor.moveRight(horz_space - m_layout_size.width);
					cursor = cursor.moveRight(space_width);
					break;
				}

				case Centre: {
					cursor = cursor.moveRight((horz_space - m_layout_size.width) / 2);
					cursor = cursor.moveRight(space_width / 2);
					break;
				}
			}

			if(not is_first_child)
				cursor = cursor.newLine(line->lineSpacing() - line->layoutSize().descent);

			if(line->requires_space_reservation())
				cursor = cursor.ensureVerticalSpace(line->layoutSize().descent);

			cursor = line->computePosition(cursor);

			// note: we want where the line *ended*
			prev_child_bottom_page = cursor.position().page_num;

			is_first_child = false;
			prev_child_was_phantom = line->is_phantom();
		}

		return cursor;
	}
}


namespace sap::tree
{
	static void flatten_para(const std::vector<zst::SharedPtr<InlineObject>>& objs,
	    std::vector<zst::SharedPtr<InlineObject>>& flattened)
	{
		for(auto& obj : objs)
		{
			if(auto span = obj->castToSpan(); span && span->canSplit())
				flatten_para(span->objects(), flattened);
			else
				flattened.push_back(obj);
		}
	}


	auto Paragraph::create_layout_object_impl(interp::Interpreter* cs,
	    const Style& parent_style,
	    Size2d available_space) const -> ErrorOr<LayoutResult>
	{
		auto _ = cs->evaluator().pushBlockContext(this);

		auto objs = TRY(this->evaluate_scripts(cs, available_space));
		if(not objs.has_value())
			return Ok(LayoutResult::empty());

		else if(objs->is_right())
			return Ok(LayoutResult::make(objs->take_right()));

		auto style = parent_style.extendWith(this->style());

		// note: the InlineSpan must continue to live here, so don't `take` it;
		// just steal its objects.
		auto tmp = std::move(objs->left()->objects());
		auto para_objects = TRY(tree::processWordSeparators(std::move(tmp)));
		if(para_objects.empty())
			return Ok(LayoutResult::empty());

		para_objects = TRY(tree::performReplacements(style, std::move(para_objects)));

		std::vector<std::unique_ptr<layout::Line>> layout_lines {};
		LayoutSize para_size {};

		// precompute line metrics (and the line bounds)
		using WordSpan = std::span<const zst::SharedPtr<tree::InlineObject>>;
		std::vector<std::pair<WordSpan, layout::LineAdjustment>> the_lines {};

		using Iter = std::vector<zst::SharedPtr<InlineObject>>::const_iterator;
		auto add_one_line = [&the_lines](auto& line, Iter words_begin, Iter words_end) {
#if 0
			zpr::print("line ({} parts): {", words_end - words_begin);

			static constexpr void (*print_obj)(const InlineObject*) = [](const InlineObject* obj) {
				if(auto t = obj->castToText())
					zpr::print("[{}]", t->contents());
				else if(auto s = obj->castToSeparator())
					zpr::print("+");
				else if(auto span = obj->castToSpan())
				{
					zpr::print("(");
					for(auto& c : span->objects())
						print_obj(c.get());
					zpr::print(")");
				}
			};

			for(auto it = words_begin; it != words_end; ++it)
				print_obj(it->get());

			zpr::println("}");
#endif
			    if((*words_begin)->isSeparator())
			    {
				    sap::internal_error(
				        "line starting with non-Word found, "
				        "either line breaking algo is broken "
				        "or we had multiple separators in a row");
			    }

			    // Ignore space at end of line
			    const auto& last_word = *(words_end - 1);
			    if(auto sep = last_word->castToSeparator(); sep && (sep->hasWhitespace()))
				    --words_end;

			    auto words_span = std::span(&*words_begin, &*words_end);
			    the_lines.push_back({
			        words_span,
			        layout::LineAdjustment {
			            .left_protrusion = line.leftProtrusion(),
			            .right_protrusion = line.rightProtrusion(),
			            .piece_adjustments = line.pieceAdjustments(),
			        },
			    });
		    };

		// break after flattening.
		std::vector<zst::SharedPtr<InlineObject>> flat {};
		flatten_para(para_objects, flat);

		auto broken_lines = layout::linebreak::breakLines(cs, style, flat, available_space.x());

		size_t current_idx = 0;
		for(auto line_it = broken_lines.begin(); line_it != broken_lines.end(); ++line_it)
		{
			auto& broken_line = *line_it;

			auto words_begin = flat.begin() + (ssize_t) current_idx;
			auto words_end = flat.begin() + (ssize_t) current_idx + (ssize_t) broken_line.numParts();

			add_one_line(broken_line, words_begin, words_end);

			current_idx += broken_line.numParts();
		}

		for(size_t i = 0; i < the_lines.size(); i++)
		{
			bool is_last_line = (i + 1 == the_lines.size());
			bool is_first_line = (i == 0);

			auto& [word_span, line_adj] = the_lines[i];

			auto layout_line = layout::Line::fromInlineObjects(cs, style, word_span, available_space, is_first_line,
			    is_last_line, line_adj);

			auto line_size = layout_line->layoutSize();

			para_size.width = std::max(para_size.width, line_size.width);
			if(is_first_line)
			{
				para_size.ascent += line_size.ascent;
				para_size.descent += line_size.descent;
			}
			else
			{
				para_size.descent += layout_line->lineSpacing();
			}

			layout_lines.push_back(std::move(layout_line));
		}

		auto layout_para = std::unique_ptr<layout::Paragraph>(new layout::Paragraph(style, para_size,
		    std::move(layout_lines), std::move(para_objects)));

		m_generated_layout_object = layout_para.get();
		return Ok(LayoutResult::make(std::move(layout_para)));
	}
}
