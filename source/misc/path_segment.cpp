// path_segment.cpp
// Copyright (c) 2023, yuki
// SPDX-License-Identifier: Apache-2.0

#include "sap/path.h"

namespace sap
{
	PathSegment::PathSegment(Kind kind) : m_kind(kind)
	{
	}

	PathSegment::PathSegment(Kind kind, Position a) : m_kind(kind)
	{
		m_points[0] = a;
	}

	PathSegment::PathSegment(Kind kind, Position a, Position b) : m_kind(kind)
	{
		m_points[0] = a;
		m_points[1] = b;
	}

	PathSegment::PathSegment(Kind kind, Position a, Position b, Position c) : m_kind(kind)
	{
		m_points[0] = a;
		m_points[1] = b;
		m_points[2] = c;
	}

	std::span<const Position> PathSegment::points() const
	{
		const size_t num_points = [this]() -> size_t {
			switch(m_kind)
			{
				case Move: return 1;
				case Line: return 1;
				case CubicBezier: return 3;
				case CubicBezierIC1: return 2;
				case CubicBezierIC2: return 2;
				case Rectangle: return 2;
				case Close: return 0;
			}
			util::unreachable();
		}();

		return std::span(m_points.begin(), num_points);
	}

	PathSegment PathSegment::move(Position to)
	{
		return PathSegment(Move, std::move(to));
	}

	PathSegment PathSegment::line(Position to)
	{
		return PathSegment(Line, std::move(to));
	}

	PathSegment PathSegment::cubicBezier(Position cp1, Position cp2, Position final)
	{
		return PathSegment(CubicBezier, std::move(cp1), std::move(cp2), std::move(final));
	}

	PathSegment PathSegment::cubicBezierIC1(Position cp2, Position final)
	{
		return PathSegment(CubicBezierIC1, std::move(cp2), std::move(final));
	}

	PathSegment PathSegment::cubicBezierIC2(Position cp1, Position final)
	{
		return PathSegment(CubicBezierIC2, std::move(cp1), std::move(final));
	}

	PathSegment PathSegment::rectangle(Position pos, Vector2 size)
	{
		return PathSegment(Rectangle, std::move(pos), Position(size.x(), size.y()));
	}

	PathSegment PathSegment::close()
	{
		return PathSegment(Close);
	}
}
