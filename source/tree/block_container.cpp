// block_container.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/container.h"

#include "layout/base.h"
#include "layout/line.h"
#include "layout/paragraph.h"

namespace sap::tree
{
	auto BlockContainer::createLayoutObject(interp::Interpreter* cs, layout::LineCursor cursor, const Style* parent_style) const
	    -> LayoutResult
	{
		auto start_cursor = cursor;
		auto style = parent_style->extendWith(m_style);

		std::vector<std::unique_ptr<layout::Line>> lines {};
		for(auto& obj : m_objects)
		{
			std::array<const BlockObject*, 1> aoeu { obj.get() };
			auto [new_cursor, line] = layout::Line::fromBlockObjects(cs, cursor, style, aoeu);

			cursor = new_cursor.newLine(style->paragraph_spacing());
			lines.push_back(std::move(line));
		}

		return { cursor, layout::Paragraph::fromLines(cs, start_cursor, style, std::move(lines)) };
	}
}
