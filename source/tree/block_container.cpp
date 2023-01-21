// block_container.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/container.h"

#include "interp/interp.h"
#include "interp/evaluator.h"

#include "layout/base.h"
#include "layout/line.h"
#include "layout/paragraph.h"

namespace sap::tree
{
	auto BlockContainer::createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style) const
	    -> LayoutResult
	{
		auto start_cursor = cursor;
		auto this_style = parent_style->extendWith(m_style);

		std::vector<std::unique_ptr<layout::Line>> lines {};

		auto layout_an_object_please = [&](BlockObject* obj) -> std::pair<bool, const Style*> {
			auto cur_style = cs->evaluator().currentStyle().unwrap()->extendWith(this_style);

			std::array<BlockObject*, 1> aoeu { obj };
			auto [new_cursor, maybe_line] = layout::Line::fromBlockObjects(cs, cursor, cur_style, aoeu);

			if(not maybe_line.has_value())
				return { false, cur_style };

			lines.push_back(std::move(*maybe_line));
			cursor = std::move(new_cursor);

			return { true, cur_style };
		};

		auto _ = cs->evaluator().pushBlockContext(cursor, this,
		    {
		        .add_block_object = [&layout_an_object_please, cs](auto obj) -> ErrorOr<void> {
			        auto ptr = &cs->leakBlockObject(std::move(obj));

			        layout_an_object_please(ptr);
			        return Ok();
		        },
		        .add_inline_object = std::nullopt,
		    });


		for(size_t i = 0; i < m_objects.size(); i++)
		{
			auto& obj = m_objects[i];

			auto [not_empty, style] = layout_an_object_please(obj.get());

			if(not_empty && i + 1 < m_objects.size())
				cursor = cursor.newLine(style->paragraph_spacing());
		}


		if(lines.empty())
			return LayoutResult::make(cursor);

		return LayoutResult::make(cursor, layout::Paragraph::fromLines(cs, start_cursor, std::move(lines)));
	}
}
