// path_segment.h
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <span>
#include <array>

#include "sap/units.h"

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
}
