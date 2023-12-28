// path_object.h
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <span>
#include <array>

#include "sap/units.h"

namespace sap
{
	struct PathObject
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

		static PathObject move(Position to);
		static PathObject line(Position to);
		static PathObject cubicBezier(Position cp1, Position cp2, Position end);
		static PathObject cubicBezierIC1(Position cp2, Position end);
		static PathObject cubicBezierIC2(Position cp1, Position end);
		static PathObject rectangle(Position pos, Vector2 size);
		static PathObject close();

	private:
		explicit PathObject(Kind kind);
		explicit PathObject(Kind kind, Position a);
		explicit PathObject(Kind kind, Position a, Position b);
		explicit PathObject(Kind kind, Position a, Position b, Position c);

		Kind m_kind;
		std::array<Position, 3> m_points;
	};

	using PathObjects = std::vector<PathObject>;
}
