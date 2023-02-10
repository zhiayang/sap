// container.cpp
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
	Container::Container(Direction direction) : m_direction(direction)
	{
	}

	std::unique_ptr<Container> Container::makeVertBox()
	{
		return std::make_unique<Container>(Direction::Vertical);
	}

	std::unique_ptr<Container> Container::makeHorzBox()
	{
		return std::make_unique<Container>(Direction::Horizontal);
	}

	std::unique_ptr<Container> Container::makeStackBox()
	{
		return std::make_unique<Container>(Direction::None);
	}


	ErrorOr<void> Container::evaluateScripts(interp::Interpreter* cs) const
	{
		for(auto& obj : m_objects)
			TRY(obj->evaluateScripts(cs));

		return Ok();
	}

	auto Container::createLayoutObject(interp::Interpreter* cs, const Style* parent_style, Size2d available_space) const
		-> ErrorOr<LayoutResult>
	{
		auto _ = cs->evaluator().pushBlockContext(this);

		auto cur_style = m_style->useDefaultsFrom(cs->evaluator().currentStyle())->useDefaultsFrom(parent_style);
		std::vector<std::unique_ptr<layout::LayoutObject>> objects;

		auto total_size = Size2d(0, 0);

		Length max_width = 0;
		Length max_height = 0;

		for(size_t i = 0; i < m_objects.size(); i++)
		{
			auto obj = TRY(m_objects[i].get()->createLayoutObject(cs, cur_style, available_space));
			if(not obj.object.has_value())
				continue;

			auto obj_size = (*obj.object)->layoutSize();

			max_width = std::max(max_width, obj_size.x());
			max_height = std::max(max_height, obj_size.y());

			objects.push_back(std::move(*obj.object));

			switch(m_direction)
			{
				using enum Direction;
				case None: {
					break;
				}

				case Vertical: {
					if(available_space.y() < obj_size.y())
					{
						sap::warn("layout", "not enough space! need at least {}, but only {} remaining", obj_size.y(),
							available_space.y());
						available_space.y() = 0;
					}
					else
					{
						available_space.y() -= obj_size.y();
					}

					break;
				}

				case Horizontal: {
					if(available_space.x() < obj_size.x())
					{
						sap::warn("layout", "not enough space! need at least {}, but only {} remaining", obj_size.x(),
							available_space.x());
						available_space.x() = 0;
					}
					else
					{
						available_space.x() -= obj_size.x();
					}

					break;
				}
			}

			total_size += obj_size;
		}

		if(objects.empty())
			return Ok(LayoutResult::empty());

		Size2d final_size {};
		layout::Container::Direction dir {};

		switch(m_direction)
		{
			using enum Direction;
			using LD = layout::Container::Direction;

			case None: {
				dir = LD::None;
				final_size = Size2d(max_width, max_height);
				break;
			}

			case Vertical: {
				dir = LD::Vertical;
				final_size = Size2d(max_width, total_size.y());
				break;
			}

			case Horizontal: {
				dir = LD::Horizontal;
				final_size = Size2d(total_size.x(), max_height);
				break;
			}
		}

		auto container = std::make_unique<layout::Container>(cur_style, final_size, dir, std::move(objects));

		m_generated_layout_object = container.get();
		return Ok(LayoutResult::make(std::move(container)));
	}
}
