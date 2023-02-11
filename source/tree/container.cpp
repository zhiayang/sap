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

		auto cur_style = m_style->useDefaultsFrom(parent_style)->useDefaultsFrom(cs->evaluator().currentStyle());
		std::vector<std::unique_ptr<layout::LayoutObject>> objects;

		LayoutSize total_size {};
		LayoutSize max_size {};

		for(size_t i = 0, obj_idx = 0; i < m_objects.size(); i++)
		{
			auto obj = TRY(m_objects[i].get()->createLayoutObject(cs, cur_style, available_space));
			if(not obj.object.has_value())
				continue;

			auto obj_size = (*obj.object)->layoutSize();

			max_size.width = std::max(max_size.width, obj_size.width);
			max_size.ascent = std::max(max_size.ascent, obj_size.ascent);
			max_size.descent = std::max(max_size.descent, obj_size.descent);

			objects.push_back(std::move(*obj.object));

			switch(m_direction)
			{
				using enum Direction;
				case None: {
					break;
				}

				case Vertical: {
					if(available_space.y() < obj_size.descent)
					{
						sap::warn("layout", "not enough space! need at least {}, but only {} remaining",
							obj_size.descent, available_space.y());

						available_space.y() = 0;
					}
					else
					{
						available_space.y() -= obj_size.descent;
					}

					break;
				}

				case Horizontal: {
					if(available_space.x() < obj_size.width)
					{
						sap::warn("layout", "not enough space! need at least {}, but only {} remaining", obj_size.width,
							available_space.x());
						available_space.x() = 0;
					}
					else
					{
						available_space.x() -= obj_size.width;
					}

					break;
				}
			}

			total_size.width += obj_size.width;
			if(obj_idx == 0)
			{
				total_size.ascent += obj_size.ascent;
				total_size.descent += obj_size.descent;
			}
			else
			{
				total_size.descent += obj_size.total_height();
			}

			obj_idx++;
		}


		if(objects.empty())
			return Ok(LayoutResult::empty());

		LayoutSize final_size {};
		Length top_to_baseline = 0;

		layout::Container::Direction dir {};

		switch(m_direction)
		{
			using enum Direction;
			using LD = layout::Container::Direction;

			case None: {
				dir = LD::None;
				top_to_baseline = max_size.ascent;
				final_size = LayoutSize {
					.width = max_size.width,
					.ascent = 0,
					.descent = max_size.total_height(),
				};
				break;
			}

			case Vertical: {
				dir = LD::Vertical;
				final_size = LayoutSize {
					.width = max_size.width,
					.ascent = 0,
					.descent = total_size.total_height(),
				};
				break;
			}

			case Horizontal: {
				dir = LD::Horizontal;
				top_to_baseline = max_size.ascent;
				final_size = LayoutSize {
					.width = total_size.width,
					.ascent = 0,
					.descent = max_size.total_height(),
				};
				break;
			}
		}

		auto container = std::make_unique<layout::Container>(cur_style, final_size, dir, top_to_baseline,
			std::move(objects));

		m_generated_layout_object = container.get();
		return Ok(LayoutResult::make(std::move(container)));
	}
}
