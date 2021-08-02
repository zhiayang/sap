// units.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <zpr.h>

namespace pdf
{
	struct Scalar
	{
		constexpr Scalar() : x(0) { }
		constexpr explicit Scalar(double x) : x(x) { }

		double x;
	};

	struct Coord
	{
		constexpr Coord() : x(0), y(0) { }
		constexpr Coord(double x, double y) : x(x), y(y) { }

		double x;
		double y;
	};


	// 72 units is 1 inch.
	static constexpr double MM_PER_UNIT = 72.0 / 25.4;
	static constexpr double MM_PER_CM   = 10.0;

	constexpr inline Scalar millimetres(double x) { return Scalar(MM_PER_UNIT * x); }
	constexpr inline Scalar centimetres(double x) { return Scalar(MM_PER_UNIT * x * MM_PER_CM); }

	constexpr inline Coord millimetres(double x, double y)
	{
		return Coord(MM_PER_UNIT * x, MM_PER_UNIT * y);
	}

	constexpr inline Coord centimetres(double x, double y)
	{
		return Coord(MM_PER_UNIT * x * MM_PER_CM, MM_PER_UNIT * y * MM_PER_CM);
	}

	constexpr inline Scalar mm(double x) { return millimetres(x); }
	constexpr inline Scalar cm(double x) { return centimetres(x); }
	constexpr inline Coord mm(double x, double y) { return millimetres(x, y); }
	constexpr inline Coord cm(double x, double y) { return centimetres(x, y); }
}


namespace zpr
{
	template <>
	struct print_formatter<pdf::Scalar>
	{
		template <typename Cb>
		void print(pdf::Scalar x, Cb&& cb, format_args args)
		{
			detail::print_one(cb, args, x.x);
		}
	};

	template <>
	struct print_formatter<pdf::Coord>
	{
		template <typename Cb>
		void print(pdf::Coord x, Cb&& cb, format_args args)
		{
			detail::print(cb, "{} {}", x.x, x.y);
		}
	};
}
