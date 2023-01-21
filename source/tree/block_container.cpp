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

		auto _ = cs->evaluator().pushBlockContext(cursor, this,
		    {
		        .add_block_object = [&lines, &cursor, &this_style, cs](auto obj) -> ErrorOr<void> {
			        auto ptr = &cs->leakBlockObject(std::move(obj));

			        std::array<BlockObject*, 1> aoeu { ptr };
			        auto style = cs->evaluator().currentStyle().unwrap()->extendWith(this_style);
			        auto [new_cursor, maybe_line] = layout::Line::fromBlockObjects(cs, cursor, style, aoeu);

			        if(not maybe_line.has_value())
				        return ErrMsg(&cs->evaluator(), "invalid object?!");

			        lines.push_back(std::move(*maybe_line));

			        cursor = new_cursor.newLine(style->paragraph_spacing());
			        return Ok();
		        },
		        .add_inline_object = std::nullopt,
		    });


		for(size_t i = 0; i < m_objects.size(); i++)
		{
			auto& obj = m_objects[i];

			auto style = cs->evaluator().currentStyle().unwrap()->extendWith(this_style);

			std::array<BlockObject*, 1> aoeu { obj.get() };
			auto [new_cursor, maybe_line] = layout::Line::fromBlockObjects(cs, cursor, style, aoeu);

			if(not maybe_line.has_value())
				continue;

			lines.push_back(std::move(*maybe_line));

			if(i + 1 < m_objects.size())
				cursor = new_cursor.newLine(style->paragraph_spacing());
			else
				cursor = std::move(new_cursor);
		}

		using Foo = std::unique_ptr<layout::LayoutObject>;

		if(lines.empty())
			return { cursor, util::vectorOf<Foo>() };

		return { cursor, util::vectorOf<Foo>(layout::Paragraph::fromLines(cs, start_cursor, std::move(lines))) };
	}
}
