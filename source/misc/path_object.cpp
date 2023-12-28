// path_object.cpp
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/path_object.h"

namespace sap
{
	PathObject::PathObject(Kind kind) : m_kind(kind)
	{
	}

	PathObject::PathObject(Kind kind, Position a) : m_kind(kind)
	{
		m_points[0] = a;
	}

	PathObject::PathObject(Kind kind, Position a, Position b) : m_kind(kind)
	{
		m_points[0] = a;
		m_points[1] = b;
	}

	PathObject::PathObject(Kind kind, Position a, Position b, Position c) : m_kind(kind)
	{
		m_points[0] = a;
		m_points[1] = b;
		m_points[2] = c;
	}

	std::span<const Position> PathObject::points() const
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
		}();

		return std::span(m_points.begin(), num_points);
	}

	PathObject PathObject::move(Position to)
	{
		return PathObject(Move, std::move(to));
	}

	PathObject PathObject::line(Position to)
	{
		return PathObject(Move, std::move(to));
	}

	PathObject PathObject::cubicBezier(Position cp1, Position cp2, Position final)
	{
		return PathObject(CubicBezier, std::move(cp1), std::move(cp2), std::move(final));
	}

	PathObject PathObject::cubicBezierIC1(Position cp2, Position final)
	{
		return PathObject(CubicBezierIC1, std::move(cp2), std::move(final));
	}

	PathObject PathObject::cubicBezierIC2(Position cp1, Position final)
	{
		return PathObject(CubicBezierIC2, std::move(cp1), std::move(final));
	}

	PathObject PathObject::rectangle(Position pos, Vector2 size)
	{
		return PathObject(Rectangle, std::move(pos), Position(size.x(), size.y()));
	}

	PathObject PathObject::close()
	{
		return PathObject(Close);
	}
}
