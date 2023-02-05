// horz_box.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/container.h"

#include "interp/interp.h"
#include "interp/evaluator.h"

#include "layout/base.h"
#include "layout/line.h"
#include "layout/container.h"
#include "layout/paragraph.h"

#include "pdf/font.h"

namespace sap::tree
{
	static sap::Length get_content_width(const Style* style, const std::vector<std::unique_ptr<layout::LayoutObject>>& objs)
	{
		// TODO: handle hbox spacing

		sap::Length width = 0;
		for(auto& obj : objs)
			width += obj->layoutSize().x();

		return width;
	}


	using TboLayoutResult = interp::OutputContext::TboLayoutResult;
	static ErrorOr<TboLayoutResult> layout_an_object_please(interp::Interpreter* cs,
	    const layout::PageCursor& start_cursor,
	    layout::PageCursor& cursor,
	    const Style* style,
	    layout::Container& container,
	    BlockObject* obj,
	    std::optional<layout::PageCursor> req_cursor)
	{
		bool should_update_cursor = not req_cursor.has_value();
		auto at_cursor = req_cursor.value_or(cursor);

		auto layout_objs = TRY(obj->createLayoutObject(cs, cursor, style));
		auto new_cursor = layout_objs.cursor;

		if(not layout_objs.object.has_value())
			return Ok<TboLayoutResult>(cursor, nullptr);

		container.addObject(std::move(*layout_objs.object));

		// now that we have a bunch of new guys, we need to reposition all the guys
		// if we are not left-aligned
		auto total_width = get_content_width(style, container.objects());
		if(style->alignment() == Alignment::Centre)
		{
			auto left_offset = (start_cursor.widthAtCursor() - total_width) / 2;
			new_cursor = start_cursor.moveRight(left_offset);
		}
		else if(style->alignment() == Alignment::Right)
		{
			auto left_offset = start_cursor.widthAtCursor() - total_width;
			new_cursor = start_cursor.moveRight(left_offset);
		}
		else if(style->alignment() == Alignment::Justified)
		{
			// TODO: not supported yet!
			sap::internal_error("justified hbox not implemented");
		}

		for(auto& obj : container.objects())
		{
			// TODO: hbox spacing
			obj->setPosition(new_cursor.position());
			new_cursor = new_cursor.moveRight(obj->layoutSize().x());
		}

		if(should_update_cursor)
			cursor = std::move(new_cursor);

		return Ok<TboLayoutResult>(cursor, &container);
	}


	ErrorOr<void> HorzBox::evaluateScripts(interp::Interpreter* cs) const
	{
		for(auto& obj : m_objects)
			TRY(obj->evaluateScripts(cs));

		return Ok();
	}

	auto HorzBox::createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style) const
	    -> ErrorOr<LayoutResult>
	{
		auto start_cursor = cursor;
		auto container = std::make_unique<layout::Container>(start_cursor.position(), Size2d());


		auto cur_style = m_style->useDefaultsFrom(cs->evaluator().currentStyle())->useDefaultsFrom(parent_style);

		auto _ = cs->evaluator().pushBlockContext(cursor, this,
		    {
		        // .add_block_object = [&](auto obj, auto at_cursor) -> ErrorOr<TboLayoutResult> {
		        //     auto ptr = m_created_block_objects.emplace_back(std::move(obj)).get();

		        //     return layout_an_object_please(cs, start_cursor, cursor, cur_style, *container, ptr, std::move(at_cursor));
		        // },
		        .set_layout_cursor = [&](auto new_cursor) -> ErrorOr<void> {
			        cursor = std::move(new_cursor);
			        return Ok();
		        },
		    });


		for(size_t i = 0; i < m_objects.size(); i++)
			TRY(layout_an_object_please(cs, start_cursor, cursor, cur_style, *container, m_objects[i].get(), std::nullopt));

		if(container->objects().empty())
			return Ok(LayoutResult::make(cursor));

		// TODO: set container size
		{
			Length max_height = 0;
			for(auto& obj : container->objects())
				max_height = std::max(obj->layoutSize().y(), max_height);

			container->setSize(Size2d(get_content_width(cur_style, container->objects()), max_height));

			cursor = cursor.ensureVerticalSpace(max_height);
			cursor = cursor.newLine(max_height);
		}

		return Ok(LayoutResult::make(cursor, std::move(container)));
	}
}
