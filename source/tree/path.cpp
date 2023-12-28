// path.cpp
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/path.h"
#include "layout/path.h"
#include "interp/interp.h"

namespace sap::tree
{
	Path::Path(std::vector<PathSegment> segments) : BlockObject(Kind::Path), m_segments()
	{
		m_segments = std::make_shared<PathSegments>(std::move(segments));
	}

	ErrorOr<void> Path::evaluateScripts(interp::Interpreter* cs) const
	{
		// no scripts to evaluate
		return Ok();
	}

	static std::pair<Position, Position> get_bezier_bounding_box(Position p1, Position p2, Position p3, Position p4)
	{
		// https://pomax.github.io/bezierinfo/#extremities
		auto va = 3.0 * (-p1 + (3.0 * p2) - (3.0 * p3) + p4);
		auto vb = 6.0 * (p1 - (2.0 * p2) + p3);
		auto vc = 3.0 * (p2 - p1);

		// want va(t^2) + vb(t) + vc = 0
		auto solve = [](double a, double b, double c, std::vector<double>& solns) {
			if(a == 0)
			{
				// solve linearly
				if(auto t = -c / b; 0.0 <= t && t <= 1.0)
					solns.push_back(t);
			}
			else
			{
				// check discriminant; make sure there's a solution.
				if(b * b - 4 * a * c < 0)
					return;

				auto s1 = (-b + std::sqrt(b * b - 4 * a * c)) / (2 * a);
				auto s2 = (-b - std::sqrt(b * b - 4 * a * c)) / (2 * a);

				if(0.0 <= s1 && s1 <= 1.0)
					solns.push_back(s1);
				if(s1 != s2 && 0.0 <= s2 && s2 <= 1.0)
					solns.push_back(s2);
			}
		};

		auto calc_bezier = [&p1, &p2, &p3, &p4](double t) -> Position {
			auto calc = [t](double a, double b, double c, double d) -> double {
				return a * 1 * (1.0 - t) * (1.0 - t) * (1.0 - t) //
				     + b * 3 * (1.0 - t) * (1.0 - t) * t         //
				     + c * 3 * (1.0 - t) * t * t                 //
				     + d * 1 * t * t * t;
			};

			return {
				calc(p1.x().value(), p2.x().value(), p3.x().value(), p4.x().value()),
				calc(p1.y().value(), p2.y().value(), p3.y().value(), p4.y().value()),
			};
		};

		// always consider the start and end points
		std::vector<double> t_values { 0.0, 1.0 };
		solve(va.x().value(), vb.x().value(), vc.x().value(), t_values);
		solve(va.y().value(), vb.y().value(), vc.y().value(), t_values);

		// now substitute in the values of T to get the points
		auto min_bounds = Position(+INFINITY, +INFINITY);
		auto max_bounds = Position(-INFINITY, -INFINITY);

		for(auto t : t_values)
		{
			auto p = calc_bezier(t);
			min_bounds.x() = std::min(min_bounds.x(), p.x());
			min_bounds.y() = std::min(min_bounds.y(), p.y());
			max_bounds.x() = std::max(max_bounds.x(), p.x());
			max_bounds.y() = std::max(max_bounds.y(), p.y());
		}

		return { min_bounds, max_bounds };
	}

	static Size2d calculate_path_size(const std::vector<PathSegment>& segments)
	{
		auto cursor = Position(0, 0);
		auto min_bounds = Position(+INFINITY, +INFINITY);
		auto max_bounds = Position(-INFINITY, -INFINITY);

		const auto update_bounds = [&](Position pos) {
			min_bounds.x() = std::min(min_bounds.x(), pos.x());
			min_bounds.y() = std::min(min_bounds.y(), pos.y());
			max_bounds.x() = std::max(max_bounds.x(), pos.x());
			max_bounds.y() = std::max(max_bounds.y(), pos.y());
		};

		const auto move_cursor = [&](Position to) {
			cursor = to;
			update_bounds(to);
		};

		for(auto& seg : segments)
		{
			using K = PathSegment::Kind;
			switch(seg.kind())
			{
				case K::Move: move_cursor(seg.points()[0]); break;
				case K::Line: move_cursor(seg.points()[0]); break;
				case K::Rectangle: update_bounds(seg.points()[0] + seg.points()[1]); break;
				case K::Close: break;

				case K::CubicBezier: {
					auto [a, b] = get_bezier_bounding_box(cursor, seg.points()[0], seg.points()[1], seg.points()[2]);
					update_bounds(a);
					update_bounds(b);
					break;
				}

				case K::CubicBezierIC1: {
					auto [a, b] = get_bezier_bounding_box(cursor, cursor, seg.points()[1], seg.points()[2]);
					update_bounds(a);
					update_bounds(b);
					break;
				}

				case K::CubicBezierIC2: {
					auto [a, b] = get_bezier_bounding_box(cursor, seg.points()[0], cursor, seg.points()[1]);
					update_bounds(a);
					update_bounds(b);
					break;
				}
			}
		}

		return max_bounds - min_bounds;
	}




	ErrorOr<LayoutResult>
	Path::create_layout_object_impl(interp::Interpreter* cs, const Style& parent_style, Size2d available_space) const
	{
		auto style = m_style.useDefaultsFrom(parent_style).useDefaultsFrom(cs->evaluator().currentStyle());
		auto path_size = calculate_path_size(*m_segments);

		return Ok(LayoutResult::make(std::make_unique<layout::Path>(style,
		    LayoutSize {
		        .width = path_size.x(),
		        .ascent = 0,
		        .descent = path_size.y(),
		    },
		    m_segments)));
	}
}
