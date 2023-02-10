// vert_box.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/container.h"

#include "interp/interp.h"
#include "interp/evaluator.h"

#include "layout/base.h"
#include "layout/line.h"
#include "layout/container.h"

#include "pdf/font.h"

namespace sap::tree
{
	ErrorOr<void> VertBox::evaluateScripts(interp::Interpreter* cs) const
	{
		for(auto& obj : m_objects)
			TRY(obj->evaluateScripts(cs));

		return Ok();
	}

	auto VertBox::createLayoutObject(interp::Interpreter* cs, const Style* parent_style, Size2d available_space) const
		-> ErrorOr<LayoutResult>
	{
		auto _ = cs->evaluator().pushBlockContext(this);

		auto cur_style = m_style->useDefaultsFrom(cs->evaluator().currentStyle())->useDefaultsFrom(parent_style);
		std::vector<std::unique_ptr<layout::LayoutObject>> objects;

		Length total_height = 0;
		Length max_width = 0;

		for(size_t i = 0; i < m_objects.size(); i++)
		{
			auto obj = TRY(m_objects[i].get()->createLayoutObject(cs, cur_style, available_space));
			if(not obj.object.has_value())
				continue;

			auto obj_width = (*obj.object)->layoutSize().x();
			auto obj_height = (*obj.object)->layoutSize().y();

			max_width = std::max(max_width, obj_width);
			objects.push_back(std::move(*obj.object));

			if(available_space.y() < obj_height)
			{
				sap::warn("layout", "not enough space! need at least {}, but only {} remaining", obj_height,
					available_space.y());
				available_space.y() = 0;
			}
			else
			{
				available_space.y() -= obj_height;
			}

			total_height += obj_height;
		}

		if(objects.empty())
			return Ok(LayoutResult::empty());

		auto container = std::make_unique<layout::Container>(cur_style, Size2d(max_width, total_height),
			layout::Container::Direction::Vertical, std::move(objects));

		m_generated_layout_object = container.get();
		return Ok(LayoutResult::make(std::move(container)));
	}
}
