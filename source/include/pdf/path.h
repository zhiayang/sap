// path.h
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <variant>

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
		virtual void addResources(const Page* page) const override;
		virtual void writePdfCommands(Stream* stream) const override;

		struct Move
		{
			Position2d position;
		};

		struct LineTo
		{
			Position2d position;
		};

		struct CubicBezier
		{
			Position2d cp1;
			Position2d cp2;
			Position2d end;
		};

		struct ClosePath
		{
		};

		using Segment = std::variant<Move, LineTo, CubicBezier, ClosePath>;

	private:
		std::vector<Segment> m_segments;
	};
}
