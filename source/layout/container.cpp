// container.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <span>

#include "layout/line.h"
#include "layout/container.h"

namespace sap::layout
{
	Container::Container(const Style& style, //
	    LayoutSize size,
	    Direction direction,
	    bool glued,
	    std::vector<std::unique_ptr<LayoutObject>> objs,
	    std::optional<Length> override_obj_spacing)
	    : LayoutObject(style, std::move(size))
	    , m_glued(glued)
	    , m_direction(direction)
	    , m_objects(std::move(objs))
	    , m_override_obj_spacing(std::move(override_obj_spacing))
	{
	}

	void Container::addObject(std::unique_ptr<LayoutObject> obj)
	{
		m_objects.push_back(std::move(obj));
	}

	std::vector<std::unique_ptr<LayoutObject>>& Container::objects()
	{
		return m_objects;
	}

	const std::vector<std::unique_ptr<LayoutObject>>& Container::objects() const
	{
		return m_objects;
	}

	bool Container::requires_space_reservation() const
	{
		switch(m_direction)
		{
			using enum Direction;

			case None:
			case Horizontal: {
				// if any of our children need it, then we need it.
				for(auto& child : m_objects)
				{
					if(child->requires_space_reservation())
						return true;
				}
				return false;
			}

			case Vertical: //
				return m_glued;
		}
		util::unreachable();
	}


	static layout::PageCursor position_children_in_container(layout::PageCursor cursor,
	    Length self_width,
	    Container::Direction direction,
	    Alignment horz_alignment,
	    Length vert_obj_spacing,
	    bool shift_by_ascent_of_first_child,
	    bool shift_by_ascent_of_remaining_children,
	    const std::vector<std::unique_ptr<LayoutObject>>& objects)
	{
		using enum Alignment;
		using enum Container::Direction;

		if(objects.empty())
			return cursor;

		auto initial_pos = cursor.position().pos;
		Length obj_spacing = 0;

		switch(direction)
		{
			case None: {
				obj_spacing = 0;
				break;
			}

			case Vertical: {
				// TODO: support vertical alignment for vbox.
				obj_spacing = vert_obj_spacing;
				break;
			}

			case Horizontal: {
				// TODO: specify inter-object spacing properly, right now there's 0.
				auto total_obj_width = std::accumulate(objects.begin(), objects.end(), Length(0),
				    [](auto a, const auto& b) { return a + b->layoutSize().width; });

				auto space_width = std::max(Length(0), self_width - total_obj_width);
				auto horz_space = cursor.widthAtCursor();

				switch(horz_alignment)
				{
					case Left: {
						obj_spacing = 0;
						break;
					}

					case Right: {
						obj_spacing = 0;
						cursor = cursor.moveRight(horz_space - self_width);
						cursor = cursor.moveRight(space_width);
						break;
					}

					case Centre: {
						obj_spacing = 0;
						cursor = cursor.moveRight((horz_space - self_width) / 2);
						cursor = cursor.moveRight(space_width / 2);
						break;
					}

					case Justified: {
						assert(not objects.empty());

						if(objects.size() == 1)
						{
							obj_spacing = 0;
						}
						else
						{
							obj_spacing = std::max(Length(0),
							    (horz_space - self_width) / static_cast<double>(objects.size() - 1));
						}

						break;
					}
				}
				break;
			}
			util::unreachable();
		}

		bool is_first_child = true;

		size_t prev_child_bottom_page = 0;
		bool prev_child_was_phantom = false;

		for(auto& child : objects)
		{
			// if we are vertically stacked, we need to move the cursor horizontally to
			// preserve horizontal alignment.
			if(direction == Vertical || direction == None)
			{
				cursor = cursor.carriageReturn();
				cursor = cursor.moveRight(initial_pos.x() - cursor.position().pos.x());

				auto horz_space = cursor.widthAtCursor();
				auto space_width = std::max(Length(0), self_width - child->layoutSize().width);

				switch(horz_alignment)
				{
					case Left:
					case Justified: break;

					case Right: {
						cursor = cursor.moveRight(horz_space - self_width);
						cursor = cursor.moveRight(space_width);
						break;
					}

					case Centre: {
						cursor = cursor.moveRight((horz_space - self_width) / 2);
						cursor = cursor.moveRight(space_width / 2);
						break;
					}
				}
			}
			else if(direction == Horizontal)
			{
				// if we're horizontally stacked, then we need to preserve vertical
				// alignment. right now, we're always aligned to the baseline.
			}


			switch(direction)
			{
				case None: {
					child->computePosition(cursor);
					break;
				}

				case Horizontal: {
					if(child->requires_space_reservation())
						cursor = cursor.ensureVerticalSpace(child->layoutSize().descent);

					child->computePosition(cursor.limitWidth(child->layoutSize().width));
					cursor = cursor.moveRight(child->layoutSize().width);

					if(not prev_child_was_phantom && not child->is_phantom())
						cursor = cursor.moveRight(obj_spacing);

					break;
				}

				case Vertical: {
					if(not is_first_child             //
					    && not prev_child_was_phantom //
					    && not child->is_phantom()    //
					    && prev_child_bottom_page == cursor.position().page_num)
					{
						/*
						    note:
						    if the previous child ended on a different page than us,
						    we don't want to add additional space.
						*/
						cursor = cursor.newLine(obj_spacing);
					}

					/*
					    first=0 shiftF=0 shiftR=0   -> 0
					    first=0 shiftF=0 shiftR=1   -> 1
					    first=0 shiftF=1 shiftR=0   -> 0
					    first=0 shiftF=1 shiftR=1   -> 1
					    first=1 shiftF=0 shiftR=0   -> 0
					    first=1 shiftF=0 shiftR=1   -> 0
					    first=1 shiftF=1 shiftR=0   -> 1
					    first=1 shiftF=1 shiftR=1   -> 1
					*/
					bool to_shift =
					    (is_first_child ? shift_by_ascent_of_first_child : shift_by_ascent_of_remaining_children);

					if(to_shift)
						cursor = cursor.newLine(child->layoutSize().ascent);

					if(child->requires_space_reservation())
						cursor = cursor.ensureVerticalSpace(child->layoutSize().descent);

					cursor = child->computePosition(cursor);

					// note: we want where the child *ended*
					prev_child_bottom_page = cursor.position().page_num;

					is_first_child = false;
					break;
				}
			}

			prev_child_was_phantom = child->is_phantom();
		}

		return cursor;
	}

	layout::PageCursor Container::compute_position_impl(layout::PageCursor cursor)
	{
		using enum Alignment;
		using enum Direction;

		this->positionRelatively(cursor.position());

		if(m_objects.empty())
			return cursor;

		auto spacing = m_override_obj_spacing.value_or(m_style.paragraph_spacing());

		cursor = position_children_in_container(cursor,       //
		    m_layout_size.width,                              //
		    m_direction,                                      //
		    m_style.horz_alignment(),                         //
		    spacing,                                          //
		    /* shift_by_ascent_of_first_child */ true,        //
		    /* shift_by_ascent_of_remaining_children */ true, //
		    m_objects);

		switch(m_direction)
		{
			case None:
			case Horizontal: { //
				return cursor.moveDown(m_layout_size.descent);
			}

			case Vertical: //
				return cursor;
		}
		util::unreachable();
	}

	void Container::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		for(auto& obj : m_objects)
			obj->render(layout, pages);
	}
}
