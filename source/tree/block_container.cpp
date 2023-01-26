// block_container.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/container.h"

#include "interp/interp.h"
#include "interp/evaluator.h"

#include "layout/base.h"
#include "layout/line.h"
#include "layout/paragraph.h"

#include "pdf/font.h"

namespace sap::tree
{
	ErrorOr<void> VertBox::evaluateScripts(interp::Interpreter* cs) const
	{
		return Ok();
	}

	auto VertBox::createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style) const
	    -> ErrorOr<LayoutResult>
	{
		bool is_first = true;

		auto start_cursor = cursor;
		std::vector<std::unique_ptr<layout::Line>> lines {};

		using TboLayoutResult = interp::OutputContext::TboLayoutResult;
		auto layout_an_object_please =
		    [&](BlockObject* obj, std::optional<layout::PageCursor> req_cursor) -> ErrorOr<TboLayoutResult> {
			auto cur_style = m_style->useDefaultsFrom(cs->evaluator().currentStyle())->useDefaultsFrom(parent_style);
			bool should_update_cursor = not req_cursor.has_value();

			auto at_cursor = req_cursor.value_or(cursor);
			if(not is_first && should_update_cursor)
				at_cursor = cursor.newLine(cur_style->paragraph_spacing()).carriageReturn();

			is_first = false;

			std::array<BlockObject*, 1> aoeu { obj };
			auto [new_cursor, maybe_line] = TRY(layout::Line::fromBlockObjects(cs, at_cursor, cur_style, aoeu));

			if(not maybe_line.has_value())
				return Ok<TboLayoutResult>(cursor, nullptr);

			auto line_ptr = lines.emplace_back(std::move(*maybe_line)).get();

			if(should_update_cursor)
				cursor = std::move(new_cursor);

			return Ok<TboLayoutResult>(cursor, line_ptr);
		};

		auto _ = cs->evaluator().pushBlockContext(cursor, this,
		    {
		        .add_block_object = [&](auto obj, auto at_cursor) -> ErrorOr<TboLayoutResult> {
			        auto ptr = m_created_block_objects.emplace_back(std::move(obj)).get();

			        return layout_an_object_please(ptr, std::move(at_cursor));
		        },
		        .set_layout_cursor = [&](auto new_cursor) -> ErrorOr<void> {
			        cursor = std::move(new_cursor);
			        return Ok();
		        },
		    });


		for(size_t i = 0; i < m_objects.size(); i++)
			TRY(layout_an_object_please(m_objects[i].get(), std::nullopt));

		if(lines.empty())
			return Ok(LayoutResult::make(cursor));

		return Ok(LayoutResult::make(cursor, layout::Paragraph::fromLines(cs, start_cursor, std::move(lines))));
	}
}
