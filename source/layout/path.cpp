// path.cpp
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/path.h"
#include "pdf/page.h"

#include "layout/path.h"

namespace sap::layout
{
	Path::Path(const Style& style,
	    LayoutSize size,
	    PathStyle path_style,
	    std::shared_ptr<PathSegments> segments,
	    Position origin)
	    : LayoutObject(style, size)
	    , m_path_style(std::move(path_style))
	    , m_segments(std::move(segments))
	    , m_origin(origin)
	{
	}

	bool Path::requires_space_reservation() const
	{
		return true;
	}

	layout::PageCursor Path::compute_position_impl(layout::PageCursor cursor)
	{
		this->positionRelatively(cursor.position());
		return cursor.moveRight(m_layout_size.width).newLine(m_layout_size.descent);
	}

	void Path::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		auto pos = this->resolveAbsPosition(layout);
		auto page = pages[pos.page_num];

		auto pdf_size = pdf::Size2d(m_layout_size.width.into(), m_layout_size.total_height().into());
		auto page_obj = util::make<pdf::Path>(page->convertVector2(pos.pos.into()), pdf_size);

		// convert our path style into a pdf paintstyle
		auto paint_style = pdf::Path::PaintStyle {
			.line_width = m_path_style.line_width.into(),
			.cap_style = m_path_style.cap_style,
			.join_style = m_path_style.join_style,
			.miter_limit = m_path_style.miter_limit,
			.stroke_colour = m_path_style.stroke_colour,
			.fill_colour = m_path_style.fill_colour,
		};

		page_obj->addSegment(std::move(paint_style));

		auto origin = pos.pos - m_origin;

		for(auto& seg : *m_segments)
		{
			using K = PathSegment::Kind;
			switch(seg.kind())
			{
				case K::Move:
					page_obj->addSegment(pdf::Path::MoveTo {
					    page->convertVector2((seg.points()[0] + origin).into()),
					});
					break;

				case K::Line:
					page_obj->addSegment(pdf::Path::LineTo {
					    page->convertVector2((seg.points()[0] + origin).into()),
					});
					break;

				case K::CubicBezier:
					page_obj->addSegment(pdf::Path::CubicBezier {
					    page->convertVector2((seg.points()[0] + origin).into()),
					    page->convertVector2((seg.points()[1] + origin).into()),
					    page->convertVector2((seg.points()[2] + origin).into()),
					});
					break;

				case K::CubicBezierIC1:
					page_obj->addSegment(pdf::Path::CubicBezierIC1 {
					    page->convertVector2((seg.points()[0] + origin).into()),
					    page->convertVector2((seg.points()[1] + origin).into()),
					});
					break;

				case K::CubicBezierIC2:
					page_obj->addSegment(pdf::Path::CubicBezierIC2 {
					    page->convertVector2((seg.points()[0] + origin).into()),
					    page->convertVector2((seg.points()[1] + origin).into()),
					});
					break;

				case K::Rectangle: {
					// note: height of rectangle must be negated because y-up/y-down conversion
					page_obj->addSegment(pdf::Path::Rectangle {
					    page->convertVector2((seg.points()[0] + origin).into()),
					    Size2d(seg.points()[1].x().into(), -seg.points()[1].y().into()).into(),
					});
					break;
				}

				case K::Close: page_obj->addSegment(pdf::Path::ClosePath {}); break;
			}
		}

		page->addObject(page_obj);
	}
}
