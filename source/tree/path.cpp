// path.cpp
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/path.h"
#include "layout/path.h"
#include "interp/interp.h"

namespace sap::tree
{
	Path::Path(PathStyle path_style, std::vector<PathSegment> segments)
	    : BlockObject(Kind::Path), m_path_style(std::move(path_style)), m_segments()
	{
		m_segments = std::make_shared<PathSegments>(std::move(segments));
	}

	ErrorOr<void> Path::evaluateScripts(interp::Interpreter* cs) const
	{
		// no scripts to evaluate
		return Ok();
	}

	using CapStyle = PathStyle::CapStyle;
	using JoinStyle = PathStyle::JoinStyle;


	static std::pair<Position, Position>
	bezier_bb(Position p1, Position p2, Position p3, Position p4, Length thickness, CapStyle cs)
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

		const auto update_bounds = [&](auto&&... xs) {
			(((min_bounds.x() = std::min(min_bounds.x(), xs.x())),    //
			     (min_bounds.y() = std::min(min_bounds.y(), xs.y())), //
			     (max_bounds.x() = std::max(max_bounds.x(), xs.x())), //
			     (max_bounds.y() = std::max(max_bounds.y(), xs.y()))),
			    ...);
		};


		for(auto t : t_values)
		{
			const auto tangent = (va * t * t + vb * t + vc).norm();
			const auto normal = Vector2(-tangent.y(), tangent.x()); // already normalised
			const auto half = (thickness / 2).value();

			const auto pt = calc_bezier(t);
			const auto x1 = pt + (normal * half);
			const auto x2 = pt + (-normal * half);

			update_bounds(pt, x1, x2);

			// if this is an endpoint, account for the cap.
			if((t == 0.0 || t == 1.0))
			{
				switch(cs)
				{
					// butt -- nothing
					case CapStyle::Butt: break;

					// it's a circle, so we can just use ±half on x and y.
					case CapStyle::Round: {
						const auto hx = Vector2(half, 0);
						const auto hy = Vector2(0, half);
						update_bounds(p1 - hx, p1 + hx, p1 - hy, p1 + hy, //
						    p4 - hx, p4 + hx, p4 - hy, p4 + hy);
						break;
					}

					// same as round, but we need to account for the square corners too
					case CapStyle::Projecting: {
						auto e1 = p1 - (tangent * half);
						auto e2 = p4 + (tangent * half);
						update_bounds(e1, e2);

						auto c1 = p1 + (-tangent * half) + (normal * half);
						auto c2 = p1 + (-tangent * half) - (normal * half);
						auto c3 = p4 + (tangent * half) + (normal * half);
						auto c4 = p4 + (tangent * half) - (normal * half);
						update_bounds(c1, c2, c3, c4);
						break;
					}
				}
			}
		}

		return { min_bounds, max_bounds };
	}

	// also returns the min bounds so we can compute the offset properly.
	static std::pair<Size2d, Position>
	calculate_path_size(const std::vector<PathSegment>& segments, Length thickness, CapStyle cs, JoinStyle js)
	{
		auto cursor = Position(0, 0);
		auto min_bounds = Position(+INFINITY, +INFINITY);
		auto max_bounds = Position(-INFINITY, -INFINITY);

		// absolutely cursed, but i love it.
		const auto update_bounds = [&](auto&&... xs) {
			(((min_bounds.x() = std::min(min_bounds.x(), xs.x())),    //
			     (min_bounds.y() = std::min(min_bounds.y(), xs.y())), //
			     (max_bounds.x() = std::max(max_bounds.x(), xs.x())), //
			     (max_bounds.y() = std::max(max_bounds.y(), xs.y()))),
			    ...);
		};

		const auto move_cursor = [&](Position to) {
			cursor = to;
			update_bounds(to);
		};

		using std::sin;
		using std::cos;

		for(auto& seg : segments)
		{
			using K = PathSegment::Kind;
			switch(seg.kind())
			{
				case K::Close: break;
				case K::Move: move_cursor(seg.points()[0]); break;

				case K::Line: {
					const auto lp1 = cursor;
					const auto lp2 = seg.points()[0];
					const auto half = (thickness / 2).value();

					// first, calculate the angle of the line.
					const auto diff = lp2 - lp1;
					const auto angle = std::atan2(diff.y().value(), diff.x().value());

					// we need 3 extra points per end of the line (so 6 total) --
					// along the vector of the line (extending out), and orthogonal to it in both directions.
					auto p2 = lp1 - Position(half * cos(angle - M_PI / 2), half * sin(angle - M_PI / 2));
					auto p3 = lp1 - Position(half * cos(angle + M_PI / 2), half * sin(angle + M_PI / 2));

					auto p5 = lp2 + Position(half * cos(angle - M_PI / 2), half * sin(angle - M_PI / 2));
					auto p6 = lp2 + Position(half * cos(angle + M_PI / 2), half * sin(angle + M_PI / 2));

					update_bounds(p2, p3, p5, p6);

					// if we are using butt caps, then ignore p1 and p4 (we are not projecting out).
					switch(cs)
					{
						// butt: nothing
						case CapStyle::Butt: break;

						// circle: see bezier above
						case CapStyle::Round: {
							const auto hx = Vector2(half, 0);
							const auto hy = Vector2(0, half);
							update_bounds(lp1 - hx, lp1 + hx, lp1 - hy, lp1 + hy, //
							    lp2 - hx, lp2 + hx, lp2 - hy, lp2 + hy);
							break;
						}

						// need to account for the corners.
						case CapStyle::Projecting: {
							const auto tangent = Vector2(cos(angle), sin(angle)).norm();
							const auto normal = Vector2(-tangent.y(), tangent.x());

							auto c1 = lp1 + (-tangent * half) + (normal * half);
							auto c2 = lp1 + (-tangent * half) - (normal * half);
							auto c3 = lp2 + (tangent * half) + (normal * half);
							auto c4 = lp2 + (tangent * half) - (normal * half);
							update_bounds(c1, c2, c3, c4);
							break;
						}
					}

					move_cursor(seg.points()[0]);
					break;
				}

				case K::Rectangle: {
					const auto half = thickness / 2;

					// rectangle is actually easy because no angles
					const auto corner = seg.points()[0] + seg.points()[1];
					update_bounds(seg.points()[0] - Position(half, half));
					update_bounds(corner + Position(half, half));

					// note: no move_cursor because the cursor does not move (it returns to the same place)
					break;
				}

				case K::CubicBezier: {
					auto [a, b] = bezier_bb(cursor, seg.points()[0], seg.points()[1], seg.points()[2], thickness, cs);
					update_bounds(a, b);
					break;
				}

				case K::CubicBezierIC1: {
					auto [a, b] = bezier_bb(cursor, cursor, seg.points()[1], seg.points()[2], thickness, cs);
					update_bounds(a, b);
					break;
				}

				case K::CubicBezierIC2: {
					auto [a, b] = bezier_bb(cursor, seg.points()[0], cursor, seg.points()[1], thickness, cs);
					update_bounds(a, b);
					break;
				}
			}
		}

		return { max_bounds - min_bounds, min_bounds };
	}




	ErrorOr<LayoutResult>
	Path::create_layout_object_impl(interp::Interpreter* cs, const Style& parent_style, Size2d available_space) const
	{
		auto style = m_style.useDefaultsFrom(parent_style).useDefaultsFrom(cs->evaluator().currentStyle());
		auto [path_size, min_bounds] = calculate_path_size(*m_segments, m_path_style.line_width, m_path_style.cap_style,
		    m_path_style.join_style);

		return Ok(LayoutResult::make(std::make_unique<layout::Path>(style,
		    LayoutSize {
		        .width = path_size.x(),
		        .ascent = 0,
		        .descent = path_size.y(),
		    },
		    m_path_style, m_segments, min_bounds)));
	}
}
