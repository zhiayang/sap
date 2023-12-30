// path.h
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <variant>

#include "sap/path.h"
#include "sap/colour.h"

#include "pdf/units.h"
#include "pdf/page_object.h"

namespace pdf
{
	struct GraphicsState
	{
	};

	struct Path : PageObject
	{
		struct MoveTo
		{
			Position2d pos;
		};

		struct LineTo
		{
			Position2d pos;
		};

		struct CubicBezier
		{
			Position2d cp1;
			Position2d cp2;
			Position2d end;
		};

		struct CubicBezierIC1
		{
			Position2d cp2;
			Position2d end;
		};

		struct CubicBezierIC2
		{
			Position2d cp1;
			Position2d end;
		};

		struct Rectangle
		{
			Position2d start;
			Size2d size;
		};

		struct ClosePath
		{
		};

		struct PaintStyle
		{
			using CapStyle = sap::PathStyle::CapStyle;
			using JoinStyle = sap::PathStyle::JoinStyle;

			PdfScalar line_width;
			CapStyle cap_style;
			JoinStyle join_style;
			double miter_limit;
			sap::Colour stroke_colour;
			sap::Colour fill_colour;
		};

		using Segment = std::
		    variant<MoveTo, LineTo, CubicBezier, CubicBezierIC1, CubicBezierIC2, Rectangle, ClosePath, PaintStyle>;

		Path(Position2d display_position, Size2d display_size);
		void addSegment(Segment segment);

		virtual void addResources(const Page* page) const override;
		virtual void writePdfCommands(Stream* stream) const override;

	private:
		Position2d m_display_position;
		Size2d m_display_size;
		std::vector<Segment> m_segments;
	};
}
