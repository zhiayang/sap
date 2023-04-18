// colour.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace sap
{
	struct Colour
	{
		enum class Type
		{
			RGB,
			CMYK,
		};

		struct RGB
		{
			double r;
			double g;
			double b;

			constexpr bool operator==(const RGB&) const = default;
			constexpr bool operator!=(const RGB&) const = default;
		};

		struct CMYK
		{
			double c;
			double m;
			double y;
			double k;

			constexpr bool operator==(const CMYK&) const = default;
			constexpr bool operator!=(const CMYK&) const = default;
		};

		constexpr bool isRGB() const { return m_type == Type::RGB; }
		constexpr bool isCMYK() const { return m_type == Type::CMYK; }

		constexpr RGB rgb() const { return m_rgb; }
		constexpr CMYK cmyk() const { return m_cmyk; }
		constexpr Type type() const { return m_type; }

		constexpr static Colour rgb(double r, double g, double b)
		{
			return Colour {
				.m_type = Type::RGB,
				.m_rgb = { .r = r, .g = g, .b = b },
			};
		}

		constexpr static Colour cmyk(double c, double m, double y, double k)
		{
			return Colour {
				.m_type = Type::CMYK,
				.m_cmyk = { .c = c, .m = m, .y = y, .k = k },
			};
		}

		constexpr static Colour grey(double grey)
		{
			return Colour {
				.m_type = Type::CMYK,
				.m_cmyk = { .c = 0, .m = 0, .y = 0, .k = grey },
			};
		}

		constexpr static Colour white() { return grey(1.0); }
		constexpr static Colour black() { return grey(0.0); }

		constexpr bool operator!=(const Colour& other) const = default;
		constexpr bool operator==(const Colour& other) const
		{
			if(m_type != other.m_type)
				return false;
			switch(m_type)
			{
				using enum Type;
				case RGB: return m_rgb == other.m_rgb;
				case CMYK: return m_cmyk == other.m_cmyk;
			}
		}



		Type m_type;
		union
		{
			struct RGB m_rgb;
			struct CMYK m_cmyk;
		};
	};
}
