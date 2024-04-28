// container.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/path.h"
#include "layout/container.h"

namespace sap::layout
{
	Container::Container(const Style& style, //
	    LayoutSize size,
	    Direction direction,
	    bool glued,
	    BorderStyle border_style,
	    std::vector<std::unique_ptr<LayoutObject>> objs,
	    std::optional<Length> override_obj_spacing)
	    : LayoutObject(style, std::move(size))
	    , m_glued(glued)
	    , m_direction(direction)
	    , m_border_style(std::move(border_style))
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

	static Length get_line_width(const std::optional<PathStyle>& s)
	{
		return s.has_value() ? s->line_width : 0;
	}

	static layout::PageCursor position_children_in_container(layout::PageCursor cursor,
	    Length self_width,
	    Container::Direction direction,
	    Alignment horz_alignment,
	    Length vert_obj_spacing,
	    bool shift_by_ascent_of_first_child,
	    bool shift_by_ascent_of_remaining_children,
	    const std::vector<std::unique_ptr<LayoutObject>>& objects,
	    const Style& cur_style,
	    const BorderStyle& border_style,
	    std::vector<layout::Path>& border_objs)
	{
		using enum Alignment;
		using enum Container::Direction;

		// yeet any cached border objects from the last thing
		border_objs.clear();

		if(objects.empty())
			return cursor;

		auto initial_pos = cursor.position().pos;
		Length obj_spacing = 0;

		/*
		    border stuff:
		    - `self_width` includes the space required for the left/right borders + padding
		*/

		const auto auxiliary_space = get_line_width(border_style.left) + border_style.left_padding.resolve(cur_style)
		                           + get_line_width(border_style.right) + border_style.right_padding.resolve(cur_style);

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
							// for jusitified, we always take the max space; however, obj-spacing
							// should exclude the space taken by the borders.
							const auto total_space = horz_space - self_width - auxiliary_space;
							obj_spacing = std::max(Length(0), total_space / static_cast<double>(objects.size() - 1));
						}

						break;
					}
				}
				break;
			}
		}

		// add the border objects now. size computation should have already figured out
		// the height of the total container and included the top/bottom border+padding.
		const auto make_hborder = [&](const PathStyle& ps) -> ErrorOr<layout::Path> {
			auto w = self_width;
			if(ps.cap_style != PathStyle::CapStyle::Butt)
				w -= ps.line_width;

			auto path = tree::Path(ps, { PathSegment::move(Position(0, 0)), PathSegment::line(Position(w, 0)) });

			return Ok(TRY(path.createLayoutObjectWithoutInterp(cur_style)));
		};

		// auto make_vborder = [&](const PathStyle& ps) -> ErrorOr<std::unique_ptr<layout::LayoutObject>> {
		// 	auto h = final_size.total_height();
		// 	if(ps.cap_style != PathStyle::CapStyle::Butt)
		// 		h -= ps.line_width;

		// 	auto path = tree::Path(ps, { PathSegment::move(Position(0, 0)), PathSegment::line(Position(0, h)) });
		// 	return Ok(*TRY(path.createLayoutObject(cs, cur_style, Size2d(INFINITY, INFINITY))).object);
		// };

		if(border_style.top)
		{
			auto& b = border_objs.emplace_back(make_hborder(*border_style.top).unwrap());

			// move the cursor to the right place. if we are vertically stacked, the border
			// should start at the leftmost content position eg:
			// =========
			//    foo
			// hahahahah
			//    bar

			switch(direction)
			{
				case None:
				case Vertical: {
					cursor = cursor.carriageReturn();
					cursor = cursor.moveRight(initial_pos.x() - cursor.position().pos.x());
					const auto horz_space = cursor.widthAtCursor() - auxiliary_space;

					switch(horz_alignment)
					{
						case Left:
						case Justified: break;

						case Right: {
							cursor = cursor.moveRight(horz_space - self_width);
							cursor = cursor.moveRight(self_width);
							break;
						}

						case Centre: {
							cursor = cursor.moveRight((horz_space - self_width) / 2);
							break;
						}
					}
					break;
				}

				case Horizontal: break;
			}

			cursor = b.computePosition(cursor);
		}

		cursor = cursor.moveDown(border_style.top_padding.resolve(cur_style));

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

				const auto horz_space = cursor.widthAtCursor() - auxiliary_space;
				const auto space_width = std::max(Length(0), self_width - child->layoutSize().width);

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
					bool to_shift = (is_first_child ? shift_by_ascent_of_first_child
					                                : shift_by_ascent_of_remaining_children);

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

		const auto initial_cursor = cursor;

		LayoutSize content_size {};
		content_size.width = m_layout_size.width;
		content_size.ascent = 0;
		content_size.descent = m_layout_size.total_height();


		// content_size.descent -= get_line_width(m_border_style.top);
		// content_size.descent -= get_line_width(m_border_style.bottom);

		// content_size.width -= get_line_width(m_border_style.left);
		// content_size.width -= get_line_width(m_border_style.right);

		// if(m_border_objects.has_value() && m_border_objects->top)
		// {
		// 	m_border_objects->top->computePosition(initial_cursor);
		// 	cursor = cursor.moveDown(m_border_objects->top->layoutSize().total_height());
		// }

		// if(m_border_objects.has_value() && m_border_objects->left)
		// {
		// 	m_border_objects->left->computePosition(initial_cursor);
		// 	cursor = cursor.moveRight(m_border_objects->left->layoutSize().width);
		// }

		// cursor = cursor.moveRight(m_border_style.left_padding.resolve(m_style))
		//              .moveDown(m_border_style.top_padding.resolve(m_style));

		auto spacing = m_override_obj_spacing.value_or(m_style.paragraph_spacing());

		// FIXME: no idea if `m_layout_size.width` below is correct, or it should be content_size?
		// we use it as the content size to determine how much to move in the x-axis to align centre/right
		// now we need to account for borders and space changes. pain.

		cursor = position_children_in_container(cursor,       //
		    m_layout_size.width,                              //
		    m_direction,                                      //
		    m_style.horz_alignment(),                         //
		    spacing,                                          //
		    /* shift_by_ascent_of_first_child */ true,        //
		    /* shift_by_ascent_of_remaining_children */ true, //
		    m_objects,                                        //
		    m_style,                                          //
		    m_border_style,                                   //
		    m_border_objects                                  //
		);

		switch(m_direction)
		{
			case None:
			case Horizontal: { //
				cursor = cursor.moveDown(m_layout_size.descent);
				break;
			}

			case Vertical: //
				break;
		}

		// cursor = cursor.moveRight(m_border_style.right_padding.resolve(m_style));
		// if(m_border_objects.has_value() && m_border_objects->right)
		// {
		// 	auto at = initial_cursor.moveRight(get_line_width(m_border_style.left)).moveRight(content_size.width);

		// 	m_border_objects->right->computePosition(at);
		// 	cursor = cursor.moveRight(m_border_objects->right->layoutSize().width);
		// }

		// cursor = cursor.moveDown(m_border_style.bottom_padding.resolve(m_style));
		// if(m_border_objects.has_value() && m_border_objects->bottom)
		// {
		// 	auto at = initial_cursor.moveDown(get_line_width(m_border_style.top)).moveDown(content_size.total_height());

		// 	m_border_objects->bottom->computePosition(at);
		// 	cursor = cursor.moveDown(m_border_objects->bottom->layoutSize().total_height());
		// }

		return cursor;
	}

	void Container::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		for(auto& obj : m_objects)
			obj->render(layout, pages);

		for(auto& p : m_border_objects)
			p.render(layout, pages);
	}
}
