// container.cpp
// Copyright (c) 2022, yuki
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

		const auto initial_pos = cursor.position().pos;
		Length obj_spacing = 0;

		const auto border_top_style = util::optional_map(border_style.top, [](auto val) {
			val.cap_style = PathStyle::CapStyle::Butt;
			return val;
		});

		const auto border_left_style = util::optional_map(border_style.left, [](auto val) {
			val.cap_style = PathStyle::CapStyle::Butt;
			return val;
		});

		const auto border_right_style = util::optional_map(border_style.right, [](auto val) {
			val.cap_style = PathStyle::CapStyle::Butt;
			return val;
		});

		const auto border_bottom_style = util::optional_map(border_style.bottom, [](auto val) {
			val.cap_style = PathStyle::CapStyle::Butt;
			return val;
		});


		/*
		    border stuff:
		    - `self_width` includes the space required for the left/right borders + padding
		*/

		const auto auxiliary_width_left = get_line_width(border_style.left)
		                                + border_style.left_padding.resolve(cur_style);
		const auto auxiliary_width_right = get_line_width(border_style.right)
		                                 + border_style.right_padding.resolve(cur_style);
		const auto auxiliary_width = auxiliary_width_left + auxiliary_width_right;
		const auto content_width = self_width - auxiliary_width;

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
							const auto total_space = horz_space - content_width;
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
			auto border = TRY(path.createLayoutObjectWithoutInterp(cur_style));

			// move the cursor to the right place. if we are vertically stacked, the border
			// should start at the leftmost content position eg:
			// =========
			//    foo
			// hahahahah
			//    bar
			// =========

			switch(direction)
			{
				case None:
				case Vertical: {
					cursor = cursor.carriageReturn();
					cursor = cursor.moveRight(initial_pos.x() - cursor.position().pos.x());
					const auto horz_space = cursor.widthAtCursor() - self_width;

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
							cursor = cursor.moveRight(horz_space / 2);
							break;
						}
					}
					break;
				}

				case Horizontal: break;
			}

			return Ok(std::move(border));
		};

		const auto make_vborder = [&](const PathStyle& ps, Length height) -> ErrorOr<layout::Path> {
			if(ps.cap_style != PathStyle::CapStyle::Butt)
				height -= ps.line_width;

			auto path = tree::Path(ps,
			    {
			        PathSegment::move(Position(0, 0)),
			        PathSegment::line(Position(0, height)),
			    });

			return Ok(TRY(path.createLayoutObjectWithoutInterp(cur_style)));
		};

		const auto left_margin = [&](PageCursor cursor) -> Length {
			switch(direction)
			{
				case None:
				case Vertical: {
					const auto horz_space = cursor.widthAtCursor() - self_width;
					switch(horz_alignment)
					{
						case Left:
						case Justified: return 0;
						case Right: return horz_space;
						case Centre: return horz_space / 2;
					}
					util::unreachable();
				}

				// if we're horizontally stacked, then we need to preserve vertical
				// alignment. right now, we're always aligned to the baseline.
				case Horizontal: return 0;
			}
			util::unreachable();
		};

		const auto top_left_pos = cursor;
		if(border_top_style)
		{
			auto& b = border_objs.emplace_back(make_hborder(*border_top_style).unwrap());
			cursor = b.computePosition(cursor);
		}

		if(const auto tp = border_style.top_padding.resolve(cur_style); tp > 0)
		{
			// if we have left or right borders, we make a short segment to cover the
			// padding area, since we draw side borders per child item
			if(border_left_style)
			{
				auto& b = border_objs.emplace_back(make_vborder(*border_left_style, tp).unwrap());

				auto p = top_left_pos.moveRight(left_margin(top_left_pos));
				if(border_top_style)
					p = p.moveDown(border_style.top->line_width);

				b.computePosition(p);
			}

			if(border_right_style)
			{
				auto& b = border_objs.emplace_back(make_vborder(*border_right_style, tp).unwrap());

				auto p = top_left_pos
				             .moveRight(left_margin(top_left_pos) + self_width - border_right_style->line_width);
				if(border_top_style)
					p = p.moveDown(border_style.top->line_width);

				b.computePosition(p);
			}

			cursor = cursor.moveDown(tp);
		}

		bool is_first_child = true;

		size_t prev_child_bottom_page = 0;
		bool prev_child_was_phantom = false;

		for(auto& child : objects)
		{
			const auto draw_left_border = [&]() {
				if(not border_left_style)
					return;

				auto& b = border_objs.emplace_back(make_vborder(*border_left_style, child->layoutSize().total_height())
				        .unwrap());
				b.computePosition(cursor);
			};

			const auto draw_right_border = [&]() {
				if(not border_right_style)
					return;

				const auto pos = cursor.moveRight(self_width - border_right_style->line_width);
				auto& b = border_objs.emplace_back(make_vborder(*border_right_style, child->layoutSize().total_height())
				        .unwrap());
				b.computePosition(pos);
			};


			// if we are vertically stacked, we need to move the cursor horizontally to
			// preserve horizontal alignment.
			switch(direction)
			{
				case None:
				case Vertical: {
					cursor = cursor.carriageReturn();
					cursor = cursor.moveRight(initial_pos.x() - cursor.position().pos.x());

					const auto space_width = std::max(Length(0), content_width - child->layoutSize().width);

					switch(horz_alignment)
					{
						case Left:
						case Justified: {
							draw_left_border();
							draw_right_border();

							cursor = cursor.moveRight(auxiliary_width_left);
							break;
						}

						case Right: {
							// note: left_margin is correctly computed for right-aligned.
							cursor = cursor.moveRight(left_margin(cursor));
							draw_left_border();
							draw_right_border();

							cursor = cursor.moveRight(auxiliary_width_left);
							cursor = cursor.moveRight(space_width);
							break;
						}

						case Centre: {
							cursor = cursor.moveRight(left_margin(cursor));
							draw_left_border();
							draw_right_border();

							cursor = cursor.moveRight(auxiliary_width_left);
							cursor = cursor.moveRight(space_width / 2);
							break;
						}
					}
					break;
				}

				case Horizontal: {
					// if we're horizontally stacked, then we need to preserve vertical
					// alignment. right now, we're always aligned to the baseline.
					break;
				}
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

		if(const auto bp = border_style.bottom_padding.resolve(cur_style); bp > 0)
		{
			const auto ref = top_left_pos.moveRight(left_margin(top_left_pos))
			                     .moveDown(cursor.position().pos.y() - top_left_pos.position().pos.y());

			// if we have left or right borders, we make a short segment to cover the
			// padding area, since we draw side borders per child item
			if(border_left_style)
			{
				auto& b = border_objs.emplace_back(make_vborder(*border_left_style, bp).unwrap());
				b.computePosition(ref);
			}

			if(border_right_style)
			{
				auto& b = border_objs.emplace_back(make_vborder(*border_right_style, bp).unwrap());
				b.computePosition(ref.moveRight(self_width - border_right_style->line_width));
			}

			cursor = cursor.moveDown(bp);
		}

		if(border_bottom_style)
		{
			auto& b = border_objs.emplace_back(make_hborder(*border_bottom_style).unwrap());
			cursor = b.computePosition(cursor);
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
