// path.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <span>
#include <array>

#include "sap/units.h"
#include "sap/colour.h"

namespace sap
{
	struct PathSegment
	{
		enum class Kind
		{
			Move = 0,       // m
			Line,           // l
			CubicBezier,    // c
			CubicBezierIC1, // v
			CubicBezierIC2, // y
			Rectangle,      // re
			Close,          // h
		};

		using enum Kind;

		Kind kind() const { return m_kind; }
		std::span<const Position> points() const;

		static PathSegment move(Position to);
		static PathSegment line(Position to);
		static PathSegment cubicBezier(Position cp1, Position cp2, Position end);
		static PathSegment cubicBezierIC1(Position cp2, Position end);
		static PathSegment cubicBezierIC2(Position cp1, Position end);
		static PathSegment rectangle(Position pos, Vector2 size);
		static PathSegment close();

	private:
		explicit PathSegment(Kind kind);
		explicit PathSegment(Kind kind, Position a);
		explicit PathSegment(Kind kind, Position a, Position b);
		explicit PathSegment(Kind kind, Position a, Position b, Position c);

		Kind m_kind;
		std::array<Position, 3> m_points;
	};

	using PathSegments = std::vector<PathSegment>;



	struct PathStyle
	{
		enum class CapStyle
		{
			Butt = 0,
			Round,
			Projecting
		};

		enum class JoinStyle
		{
			Miter = 0,
			Round,
			Bevel
		};

		// these are the defaults as specified in PDF
		Length line_width = dim::Scalar<dim::units::pdf_user_unit>(1.0).into();
		CapStyle cap_style = CapStyle::Butt;
		JoinStyle join_style = JoinStyle::Miter;
		double miter_limit = 10;

		Colour stroke_colour = Colour::black();
		Colour fill_colour = Colour::black();
	};

	// TODO: support rounded corners maybe?
	struct BorderStyle
	{
		std::optional<PathStyle> left {};
		std::optional<PathStyle> right {};
		std::optional<PathStyle> top {};
		std::optional<PathStyle> bottom {};

		DynLength left_padding {};
		DynLength right_padding {};
		DynLength top_padding {};
		DynLength bottom_padding {};
	};
}
