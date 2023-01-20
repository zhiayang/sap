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
		auto _ = cs->evaluator().pushBlockContext({ .origin = cursor, .parent = this });

		auto start_cursor = cursor;
		auto this_style = parent_style->extendWith(m_style);

		std::vector<std::unique_ptr<layout::Line>> lines {};
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

		if(lines.empty())
			return { cursor, std::nullopt };

		return { cursor, layout::Paragraph::fromLines(cs, start_cursor, std::move(lines)) };
	}
}
