// path.cpp
// Copyright (c) 2023, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/path.h"
#include "layout/path.h"
#include "interp/interp.h"

#if defined(M_PI)
static constexpr double PI = M_PI;
#else
static constexpr double PI = 3.14159265358979323846264338327950;
#endif


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

	// absolutely cursed, but i love it.
	[[nodiscard]] static auto update_bounds(Position& min, Position& max)
	{
		return [&min, &max](auto&&... xs) {
			(((min.x() = std::min(min.x(), xs.x())),    //
			     (min.y() = std::min(min.y(), xs.y())), //
			     (max.x() = std::max(max.x(), xs.x())), //
			     (max.y() = std::max(max.y(), xs.y()))),
			    ...);
		};
	}


	static std::tuple<Position, Position, Vector2, Vector2> get_bezier_bounding_box(Position p1,
	    Position p2,
	    Position p3,
	    Position p4,
	    bool start_cap,
	    bool end_cap,
	    Length thickness,
	    CapStyle cs)
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
			return p1 * 1 * (1.0 - t) * (1.0 - t) * (1.0 - t) //
			     + p2 * 3 * (1.0 - t) * (1.0 - t) * t         //
			     + p3 * 3 * (1.0 - t) * t * t                 //
			     + p4 * 1 * t * t * t;
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
			const auto tangent = (va * t * t + vb * t + vc).norm();
			const auto normal = tangent.perpendicular();
			const auto half = (thickness / 2).value();

			const auto pt = calc_bezier(t);
			const auto x1 = pt + (normal * half);
			const auto x2 = pt + (-normal * half);

			update_bounds(min_bounds, max_bounds)(pt, x1, x2);

			// if this is an endpoint, account for the cap.
			if(start_cap && t == 0.0)
			{
				switch(cs)
				{
					// butt -- nothing
					case CapStyle::Butt: break;

					// it's a circle, so we can just use ±half on x and y.
					case CapStyle::Round: {
						const auto hx = Vector2(half, 0);
						const auto hy = Vector2(0, half);
						update_bounds(min_bounds, max_bounds)(p1 - hx, p1 + hx, p1 - hy, p1 + hy);
						break;
					}

					// same as round, but we need to account for the square corners too
					case CapStyle::Projecting: {
						auto e1 = p1 - (tangent * half);
						auto c1 = p1 + (-tangent * half) + (normal * half);
						auto c2 = p1 + (-tangent * half) - (normal * half);
						update_bounds(min_bounds, max_bounds)(e1, c1, c2);
						break;
					}
				}
			}
			else if(end_cap && t == 1.0)
			{
				switch(cs)
				{
					// butt -- nothing
					case CapStyle::Butt: break;

					// it's a circle, so we can just use ±half on x and y.
					case CapStyle::Round: {
						const auto hx = Vector2(half, 0);
						const auto hy = Vector2(0, half);
						update_bounds(min_bounds, max_bounds)(p4 - hx, p4 + hx, p4 - hy, p4 + hy);
						break;
					}

					// same as round, but we need to account for the square corners too
					case CapStyle::Projecting: {
						auto e2 = p4 + (tangent * half);
						auto c3 = p4 + (tangent * half) + (normal * half);
						auto c4 = p4 + (tangent * half) - (normal * half);
						update_bounds(min_bounds, max_bounds)(e2, c3, c4);
						break;
					}
				}
			}
		}

		return { min_bounds, max_bounds, vc.norm(), (va * 1 * 1 + vb * 1 + vc).norm() };
	}


	static std::tuple<Position, Position, Vector2, Vector2> get_line_bounding_box(Position lp1,
	    Position lp2,
	    bool start_cap,
	    bool end_cap,
	    Length thickness,
	    CapStyle cs)
	{
		auto min_bounds = Position(+INFINITY, +INFINITY);
		auto max_bounds = Position(-INFINITY, -INFINITY);

		const auto half = (thickness / 2).value();

		// first, calculate the angle of the line.
		const auto diff = lp2 - lp1;
		const auto angle = std::atan2(diff.y().value(), diff.x().value());

		// we need 3 extra points per end of the line (so 6 total) --
		// along the vector of the line (extending out), and orthogonal to it in both directions.
		auto p2 = lp1 - Position(half * cos(angle - PI / 2), half * sin(angle - PI / 2));
		auto p3 = lp1 - Position(half * cos(angle + PI / 2), half * sin(angle + PI / 2));

		auto p5 = lp2 + Position(half * cos(angle - PI / 2), half * sin(angle - PI / 2));
		auto p6 = lp2 + Position(half * cos(angle + PI / 2), half * sin(angle + PI / 2));

		update_bounds(min_bounds, max_bounds)(p2, p3, p5, p6);

		// if we are using butt caps, then ignore p1 and p4 (we are not projecting out).
		if(start_cap || end_cap)
		{
			switch(cs)
			{
				// butt: nothing
				case CapStyle::Butt: break;

				// circle: see bezier above
				case CapStyle::Round: {
					const auto hx = Vector2(half, 0);
					const auto hy = Vector2(0, half);
					if(start_cap)
						update_bounds(min_bounds, max_bounds)(lp1 - hx, lp1 + hx, lp1 - hy, lp1 + hy);
					if(end_cap)
						update_bounds(min_bounds, max_bounds)(lp2 - hx, lp2 + hx, lp2 - hy, lp2 + hy);

					break;
				}

				// need to account for the corners.
				case CapStyle::Projecting: {
					const auto tangent = Vector2(cos(angle), sin(angle)).norm();
					const auto normal = tangent.perpendicular();

					auto c1 = lp1 + (-tangent * half) + (normal * half);
					auto c2 = lp1 + (-tangent * half) - (normal * half);
					auto c3 = lp2 + (tangent * half) + (normal * half);
					auto c4 = lp2 + (tangent * half) - (normal * half);
					if(start_cap)
						update_bounds(min_bounds, max_bounds)(c1, c2);
					if(end_cap)
						update_bounds(min_bounds, max_bounds)(c3, c4);
					break;
				}
			}
		}

		// in and out direction is the same vector (ie. same direction as the line itself)
		return { min_bounds, max_bounds, diff.norm(), diff.norm() };
	}



	// also returns the min bounds so we can compute the offset properly.
	static std::pair<Size2d, Position> calculate_path_size(const std::vector<PathSegment>& segments,
	    Length thickness,
	    CapStyle cs,
	    JoinStyle js,
	    double miter_limit)
	{
		auto cursor = Position(0, 0);
		auto prev_cursor = cursor;

		Vector2 in_dir {};
		Vector2 out_dir {};
		Vector2 prev_out_dir {};

		auto min_bounds = Position(+INFINITY, +INFINITY);
		auto max_bounds = Position(-INFINITY, -INFINITY);

		const auto move_cursor = [&](Position to) {
			prev_cursor = cursor;
			cursor = to;
			update_bounds(min_bounds, max_bounds)(to);
		};

		const auto update_directions = [&](Vector2 in, Vector2 out) {
			prev_out_dir = out_dir;
			out_dir = out;
			in_dir = in;
		};

		using std::sin;
		using std::cos;
		using K = PathSegment::Kind;

		const auto need_cap = [&segments](size_t neighbour_idx) -> bool {
			switch(segments[neighbour_idx].kind())
			{
				case K::Close:
				case K::Move:
				case K::Rectangle: //
					return true;

				case K::Line:
				case K::CubicBezier:
				case K::CubicBezierIC1:
				case K::CubicBezierIC2: //
					return false;
			}

			util::unreachable();
		};

		bool joined_to_prev = false;
		for(size_t i = 0; i < segments.size(); i++)
		{
			bool start_cap = (i == 0) || need_cap(i - 1);
			bool end_cap = (i + 1 == segments.size()) || need_cap(i + 1);

			auto& seg = segments[i];

			switch(seg.kind())
			{
				case K::Close: joined_to_prev = false; break;
				case K::Move:
					move_cursor(seg.points()[0]);
					joined_to_prev = false;
					break;

				case K::Line: {
					auto [a, b, idir, odir] = get_line_bounding_box(cursor, seg.points()[0], //
					    start_cap, end_cap, thickness, cs);
					move_cursor(seg.points()[0]);
					update_bounds(min_bounds, max_bounds)(a, b);
					update_directions(idir, odir);
					break;
				}

				case K::Rectangle: {
					const auto half = thickness / 2;

					// rectangle is actually easy because no angles
					const auto corner = seg.points()[0] + seg.points()[1];
					update_bounds(min_bounds, max_bounds)(seg.points()[0] - Position(half, half));
					update_bounds(min_bounds, max_bounds)(corner + Position(half, half));

					// note: no move_cursor because the cursor does not move (it returns to the same place)
					joined_to_prev = false;
					break;
				}

				case K::CubicBezier: {
					auto [a, b, idir, odir] = get_bezier_bounding_box(cursor, seg.points()[0], seg.points()[1],
					    seg.points()[2], start_cap, end_cap, thickness, cs);
					update_bounds(min_bounds, max_bounds)(a, b);
					move_cursor(seg.points()[2]);
					update_directions(idir, odir);
					break;
				}

				case K::CubicBezierIC1: {
					auto [a, b, idir, odir] = get_bezier_bounding_box(cursor, cursor, seg.points()[0], seg.points()[1],
					    start_cap, end_cap, thickness, cs);
					update_bounds(min_bounds, max_bounds)(a, b);
					move_cursor(seg.points()[1]);
					update_directions(idir, odir);
					break;
				}

				case K::CubicBezierIC2: {
					auto [a, b, idir, odir] = get_bezier_bounding_box(cursor, seg.points()[0], cursor, seg.points()[1],
					    start_cap, end_cap, thickness, cs);
					update_bounds(min_bounds, max_bounds)(a, b);
					move_cursor(seg.points()[1]);
					update_directions(idir, odir);
					break;
				}
			}

			// if we drew a start cap, then there is no join logic.
			if(start_cap)
				continue;

			switch(js)
			{
				// bevel: the "corners" of each segment already bounds the bevel, so there's
				// nothing to do.
				case JoinStyle::Bevel: break;

				// simplest case; just extend by half thickness in all directions.
				case JoinStyle::Round: {
					const auto hx = Vector2(thickness / 2, 0);
					const auto hy = Vector2(0, thickness / 2);

					update_bounds(min_bounds, max_bounds)(prev_cursor - hx, prev_cursor + hx, prev_cursor - hy,
					    prev_cursor + hy);
					break;
				}

				case JoinStyle::Miter: {
					// get the angle between the two lines; negate one of them because angles
					const auto dot = prev_out_dir.dot(-in_dir);
					const auto angle = std::acos(dot / (prev_out_dir.magnitude().value() * in_dir.magnitude()));

					// spec definition:
					// ratio == miter_length / line_width == 1 / sin(angle/2)
					auto ratio = (1.0 / sin(angle / 2));
					if(ratio <= miter_limit)
					{
						const auto miter_length = thickness * ratio;
						const auto normal = (prev_out_dir + in_dir).perpendicular().norm();
						const auto half = (normal * miter_length) / 2;

						update_bounds(min_bounds, max_bounds)(prev_cursor + half, prev_cursor - half);
					}
					break;
				}
			}
		}

		return { max_bounds - min_bounds, min_bounds };
	}




	ErrorOr<LayoutResult> Path::create_layout_object_impl(interp::Interpreter* cs,
	    const Style& parent_style,
	    Size2d available_space) const
	{
		auto style = m_style.useDefaultsFrom(parent_style).useDefaultsFrom(cs->evaluator().currentStyle());
		return Ok(LayoutResult::make(std::make_unique<
		    layout::Path>(TRY(this->createLayoutObjectWithoutInterp(style)))));
	}

	ErrorOr<layout::Path> Path::createLayoutObjectWithoutInterp(const Style& final_style) const
	{
		auto [path_size, min_bounds] = calculate_path_size(*m_segments, m_path_style.line_width, m_path_style.cap_style,
		    m_path_style.join_style, m_path_style.miter_limit);

		// zpr::println("size: {}, bounds: {}", path_size, min_bounds);
		return Ok<layout::Path>(final_style,
		    LayoutSize {
		        .width = path_size.x(),
		        .ascent = 0,
		        .descent = path_size.y(),
		    },
		    m_path_style, m_segments, min_bounds);
	}
}
