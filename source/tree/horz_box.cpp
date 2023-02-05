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
	ErrorOr<void> HorzBox::evaluateScripts(interp::Interpreter* cs) const
	{
		for(auto& obj : m_objects)
			TRY(obj->evaluateScripts(cs));

		return Ok();
	}

	auto HorzBox::createLayoutObject(interp::Interpreter* cs, const Style* parent_style, Size2d available_space) const
		-> ErrorOr<LayoutResult>
	{
		auto _ = cs->evaluator().pushBlockContext(this);

		auto cur_style = m_style->useDefaultsFrom(cs->evaluator().currentStyle())->useDefaultsFrom(parent_style);
		std::vector<std::unique_ptr<layout::LayoutObject>> objects;

		Length total_width = 0;
		Length max_height = 0;

		for(size_t i = 0; i < m_objects.size(); i++)
		{
			auto obj = TRY(m_objects[i].get()->createLayoutObject(cs, cur_style, available_space));
			if(not obj.object.has_value())
				continue;

			auto obj_width = (*obj.object)->layoutSize().x();
			auto obj_height = (*obj.object)->layoutSize().y();

			max_height = std::max(max_height, obj_height);
			objects.push_back(std::move(*obj.object));

			if(available_space.x() < obj_width)
			{
				sap::warn("layout", "not enough space! need at least {}, but only {} remaining", obj_width, available_space.x());
				available_space.x() = 0;
			}
			else
			{
				available_space.x() -= obj_width;
			}

			total_width += obj_width;
		}

		if(objects.empty())
			return Ok(LayoutResult::empty());

		return Ok(LayoutResult::make(std::make_unique<layout::Container>(Size2d(total_width, max_height))));
	}
}
