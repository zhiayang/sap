// container.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/path.h"
#include "tree/container.h"

#include "interp/interp.h"
#include "interp/evaluator.h"

#include "layout/base.h"
#include "layout/line.h"
#include "layout/container.h"
#include "layout/paragraph.h"

namespace sap::tree
{
	Container::Container(Direction direction, bool glued)
	    : BlockObject(Kind::Container), m_glued(glued), m_direction(direction)
	{
		this->setStyle(m_style.with_horz_alignment(Alignment::Left));
	}

	zst::SharedPtr<Container> Container::makeVertBox()
	{
		return zst::make_shared<Container>(Direction::Vertical);
	}

	zst::SharedPtr<Container> Container::makeHorzBox()
	{
		return zst::make_shared<Container>(Direction::Horizontal);
	}

	zst::SharedPtr<Container> Container::makeStackBox()
	{
		return zst::make_shared<Container>(Direction::None);
	}


	ErrorOr<void> Container::evaluateScripts(interp::Interpreter* cs) const
	{
		for(auto& obj : m_objects)
			TRY(obj->evaluateScripts(cs));

		return Ok();
	}

	auto Container::create_layout_object_impl(interp::Interpreter* cs,
	    const Style& parent_style,
	    Size2d available_space) const -> ErrorOr<LayoutResult>
	{
		auto _ = cs->evaluator().pushBlockContext(this);

		auto cur_style = m_style.useDefaultsFrom(parent_style).useDefaultsFrom(cs->evaluator().currentStyle());
		std::vector<std::unique_ptr<layout::LayoutObject>> objects;

		bool have_top_border = false;
		bool have_left_border = false;
		bool have_right_border = false;
		bool have_bottom_border = false;

		bool prev_child_was_phantom = false;
		LayoutSize total_size {};

		Length extra_border_width = 0;
		Length extra_border_height = 0;

		if(m_border_style.top.has_value() && m_border_style.top->line_width > 0)
		{
			extra_border_height += m_border_style.top->line_width;
			have_top_border = true;
		}

		if(m_border_style.left.has_value() && m_border_style.left->line_width > 0)
		{
			extra_border_width += m_border_style.left->line_width;
			have_left_border = true;
		}

		if(m_border_style.right.has_value() && m_border_style.right->line_width > 0)
		{
			extra_border_width += m_border_style.right->line_width;
			have_right_border = true;
		}

		extra_border_width += m_border_style.left_padding.resolve(cur_style);
		extra_border_width += m_border_style.right_padding.resolve(cur_style);

		extra_border_height += m_border_style.top_padding.resolve(cur_style);
		extra_border_height += m_border_style.bottom_padding.resolve(cur_style);


		available_space.x() -= extra_border_width;
		available_space.y() -= extra_border_height;

		auto max_size = total_size;
		for(size_t i = 0, obj_idx = 0; i < m_objects.size(); i++)
		{
			auto obj = TRY(m_objects[i].get()->createLayoutObject(cs, cur_style, available_space));
			if(not obj.object.has_value())
				continue;

			auto obj_size = (*obj.object)->layoutSize();
			bool is_phantom = (*obj.object)->is_phantom();

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
						// sap::warn("layout", "not enough space! need at least {.15f}, but only {.15f} remaining",
						// 	obj_size.width, available_space.x());
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

			// if we had a top border, our ascent should be 0.
			if(obj_idx == 0 && not have_top_border)
			{
				total_size.ascent += obj_size.ascent;
				total_size.descent += obj_size.descent;
			}
			else
			{
				if(not prev_child_was_phantom && not is_phantom)
					total_size.descent += cur_style.paragraph_spacing();

				total_size.descent += obj_size.total_height();
			}

			prev_child_was_phantom = is_phantom;
			obj_idx++;
		}

		// FIXME: we might want to handle the case where we set a fixed size? we should probably still
		// lay *something* out. especially if we have borders, we should draw those.
		if(objects.empty())
			return Ok(LayoutResult::empty());

		if(m_border_style.bottom.has_value() && m_border_style.bottom->line_width > 0)
		{
			extra_border_height += m_border_style.bottom->line_width;
			have_bottom_border = true;
		}

		LayoutSize final_size {};
		layout::Container::Direction dir {};

		switch(m_direction)
		{
			using enum Direction;
			using LD = layout::Container::Direction;

			case None: {
				dir = LD::None;
				final_size = LayoutSize {
					.width = max_size.width,
					.ascent = max_size.ascent,
					.descent = max_size.descent,
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
				final_size = LayoutSize {
					.width = total_size.width,
					.ascent = max_size.ascent,
					.descent = max_size.descent,
				};
				break;
			}
		}

		final_size.width += extra_border_width;
		final_size.descent += extra_border_height;

		layout::BorderObjects border_objs {};

		// TODO: this should move to layout/container.cpp, when we are placing the children. if the
		// children overflow a page, then we need 6-8 lines (for 2 pages), so obviously storing the
		// border objects a-priori is not a good idea. that does mean we need mutable state in the
		// layout::Container (to store the objects there between computePosition() and render()), which sucks

		// TODO: borders are generally fucked; consider horz/vert alignment; we want the borders to
		// fit around the content (eg. centre alignment -- the border is a box in the middle of the page).
		//
		// also, i suppose if the alignment style is Justified, then the border should take up the whole
		// page width; if left/right/centre it should be tight around the content and move with it.

		// note: due to the slightly weird way that we handle positioning path objects, the top-left corner
		// of the entire path object is always the leftmost filled point in the tight bounding box, no matter
		// where the actual coordinates are. eg. `moveto(1000, 1000); lineto(1010, 1010)` is equivalent to
		// `lineto(10, 10)`. kinda cursed but whatever.

		auto make_hborder = [&](const PathStyle& ps) -> ErrorOr<std::unique_ptr<layout::LayoutObject>> {
			auto w = final_size.width;
			if(ps.cap_style != PathStyle::CapStyle::Butt)
				w -= ps.line_width;

			auto path = tree::Path(ps, { PathSegment::move(Position(0, 0)), PathSegment::line(Position(w, 0)) });

			return Ok(*TRY(path.createLayoutObject(cs, cur_style, Size2d(INFINITY, INFINITY))).object);
		};

		auto make_vborder = [&](const PathStyle& ps) -> ErrorOr<std::unique_ptr<layout::LayoutObject>> {
			auto h = final_size.total_height();
			if(ps.cap_style != PathStyle::CapStyle::Butt)
				h -= ps.line_width;

			auto path = tree::Path(ps, { PathSegment::move(Position(0, 0)), PathSegment::line(Position(0, h)) });
			return Ok(*TRY(path.createLayoutObject(cs, cur_style, Size2d(INFINITY, INFINITY))).object);
		};


		if(have_top_border)
			border_objs.top = TRY(make_hborder(*m_border_style.top));

		if(have_left_border)
			border_objs.left = TRY(make_vborder(*m_border_style.left));

		if(have_right_border)
			border_objs.right = TRY(make_vborder(*m_border_style.right));

		if(have_bottom_border)
			border_objs.bottom = TRY(make_hborder(*m_border_style.bottom));


		auto container = std::make_unique<layout::Container>(cur_style, final_size, dir, m_glued, m_border_style,
		    std::move(objects), std::move(border_objs));

		return Ok(LayoutResult::make(std::move(container)));
	}
}
