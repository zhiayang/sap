// units.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <zpr.h>



namespace dim
{
	/*
	    Both `Scalar` and `Vector` operate on the same principle. The first template parameter
	    is simply a tag, so that we do not (and cannot) confuse / interoperate values using
	    different base units.

	    For instance, in the PDF layer, we might want to operate with typographic units (72 units = 1 inch),
	    but everywhere else we probably want to stick to mm or some other metric unit. However, we do define
	    the notion of a "elementary" unit, such that all derivative units must be scaled in terms of it.

	    This allows (explicit) conversions between different measurement systems.
	*/

	template <typename _System, typename _Type = double>
	struct Scalar
	{
		using value_type = _Type;
		using unit_system = _System;
		using self_type = Scalar<_System, _Type>;

		static constexpr auto scale_factor = _System::scale_factor;
		static_assert(scale_factor != 0);

		constexpr Scalar() : _x(0) { }
		constexpr explicit Scalar(value_type x) : _x(x) { }

		constexpr Scalar(self_type&&) = default;
		constexpr Scalar(const self_type&) = default;
		constexpr self_type& operator=(self_type&&) = default;
		constexpr self_type& operator=(const self_type&) = default;

		constexpr bool zero() const { return this->_x == 0; }

		constexpr value_type& value() { return this->_x; }
		constexpr value_type value() const { return this->_x; }

		constexpr value_type& x() { return this->_x; }
		constexpr value_type x() const { return this->_x; }

		constexpr self_type operator-() const { return self_type(-this->_x); }

		constexpr self_type& operator+=(self_type ofs)
		{
			this->_x += ofs._x;
			return *this;
		}
		constexpr self_type& operator-=(self_type ofs)
		{
			this->_x -= ofs._x;
			return *this;
		}
		constexpr self_type& operator*=(value_type scale)
		{
			this->_x *= scale;
			return *this;
		}
		constexpr self_type& operator/=(value_type scale)
		{
			this->_x /= scale;
			return *this;
		}

		constexpr bool operator==(const self_type& other) const { return this->_x == other._x; }
		constexpr bool operator!=(const self_type& other) const { return this->_x != other._x; }
		constexpr bool operator<=(const self_type& other) const { return this->_x <= other._x; }
		constexpr bool operator>=(const self_type& other) const { return this->_x >= other._x; }
		constexpr bool operator<(const self_type& other) const { return this->_x < other._x; }
		constexpr bool operator>(const self_type& other) const { return this->_x > other._x; }

		template <typename _S, typename _T>
		constexpr Scalar<_S, _T> into(Scalar<_S, _T> foo) const
		{
			return Scalar<_S, _T>((this->_x * scale_factor) / _S::scale_factor);
		}

		template <>
		constexpr self_type into<unit_system, value_type>(self_type foo) const
		{
			return *this;
		}


		template <typename _Target>
		constexpr Scalar<_Target> into(_Target foo) const
		{
			return Scalar<_Target>((this->_x * scale_factor) / _Target::scale_factor);
		}

		value_type _x;
	};

	template <typename _System, typename _Type = double>
	struct Vector2
	{
		using value_type = _Type;
		using unit_system = _System;
		using scalar_type = Scalar<_System, _Type>;
		using self_type = Vector2<_System, _Type>;

		static constexpr auto scale_factor = _System::scale_factor;
		static_assert(scale_factor != 0);

		constexpr Vector2() : _x(0), _y(0) { }
		constexpr Vector2(value_type x, value_type y) : _x(scalar_type(x)), _y(scalar_type(y)) { }
		constexpr Vector2(scalar_type x, scalar_type y) : _x(x), _y(y) { }

		constexpr Vector2(self_type&&) = default;
		constexpr Vector2(const self_type&) = default;
		constexpr self_type& operator=(self_type&&) = default;
		constexpr self_type& operator=(const self_type&) = default;

		constexpr bool zero() const { return this->_x == 0 && this->_y == 0; }

		constexpr self_type operator-() const { return self_type(-this->_x, -this->_y); }

		constexpr self_type& operator+=(self_type ofs)
		{
			this->_x += ofs._x;
			this->_y += ofs._y;
			return *this;
		}
		constexpr self_type& operator-=(self_type ofs)
		{
			this->_x -= ofs._x;
			this->_y -= ofs._y;
			return *this;
		}
		constexpr self_type& operator*=(value_type scale)
		{
			this->_x *= scale;
			this->_y *= scale;
			return *this;
		}
		constexpr self_type& operator/=(value_type scale)
		{
			this->_x /= scale;
			this->_y /= scale;
			return *this;
		}

		// we don't define >/</>=/<=, because sometimes we want (x1 > x2 && y1 > y2), but sometimes
		// we want `||` instead. better to just make it explicit in the code.
		constexpr bool operator==(const self_type& other) const { return this->_x == other._x && this->_y == other._y; }
		constexpr bool operator!=(const self_type& other) const { return !(*this == other); }

		template <typename _S, typename _T>
		constexpr Vector2<_S, _T> into(Vector2<_S, _T> foo) const
		{
			return Vector2<_S, _T>(((this->_x * scale_factor) / _S::scale_factor)._x,
				((this->_y * scale_factor) / _S::scale_factor)._x);
		}

		template <>
		constexpr self_type into<unit_system, value_type>(self_type foo) const
		{
			return *this;
		}


		template <typename _Target>
		constexpr Vector2<_Target> into(_Target foo) const
		{
			return Vector2<_Target>((this->_x * scale_factor) / _Target::scale_factor,
				(this->_y * scale_factor) / _Target::scale_factor);
		}

		constexpr const scalar_type& x() const { return this->_x; }
		constexpr const scalar_type& y() const { return this->_y; }

		constexpr scalar_type& x() { return this->_x; }
		constexpr scalar_type& y() { return this->_y; }

		scalar_type _x;
		scalar_type _y;
	};

	namespace units
	{
		/*
		    Here, we define the base unit (where the scale factor is 1).
		*/
		struct base_unit
		{
			static constexpr double scale_factor = 1.0;
		};
	}
}

/*
    due to various reasons, all units must be defined in the global namespace. use
    type aliases to bring them into an inner namespace. this also means that the names
    cannot collide.
*/
#define DEFINE_UNIT(_unit_name, _SF)                                               \
	namespace dim::units                                                           \
	{                                                                              \
		struct _unit_name                                                          \
		{                                                                          \
			static constexpr double scale_factor = _SF;                            \
		};                                                                         \
	}                                                                              \
	namespace dim                                                                  \
	{                                                                              \
		constexpr inline Scalar<units::_unit_name> _unit_name(double x)            \
		{                                                                          \
			return Scalar<units::_unit_name>(x);                                   \
		}                                                                          \
		constexpr inline Vector2<units::_unit_name> _unit_name(double x, double y) \
		{                                                                          \
			return Vector2<units::_unit_name>(x, y);                               \
		}                                                                          \
	}

#define DEFINE_UNIT_AND_ALIAS(_unit_name, _SF, _alias_name) \
	DEFINE_UNIT(_unit_name, _SF)                            \
	DEFINE_UNIT(_alias_name, _SF)

#define DEFINE_UNIT_IN_NAMESPACE(_unit_name, _SF, _namespace, _name_in_namespace)                    \
	DEFINE_UNIT(_unit_name, _SF)                                                                     \
	namespace _namespace                                                                             \
	{                                                                                                \
		constexpr inline dim::Scalar<dim::units::_unit_name> _name_in_namespace(double x)            \
		{                                                                                            \
			return dim::Scalar<dim::units::_unit_name>(x);                                           \
		}                                                                                            \
		constexpr inline dim::Vector2<dim::units::_unit_name> _name_in_namespace(double x, double y) \
		{                                                                                            \
			return dim::Vector2<dim::units::_unit_name>(x, y);                                       \
		}                                                                                            \
	}

/*
    all other units should be in terms of the base unit; so we have
    1mm = 1bu, 1cm = 10bu, etc.
*/
DEFINE_UNIT_AND_ALIAS(millimetre, 1.0, mm);
DEFINE_UNIT_AND_ALIAS(centimetre, 10.0, cm);



#define DELETE_CONVERSION_VECTOR2(_FromUnit, _ToUnit)                                                                      \
	namespace dim                                                                                                          \
	{                                                                                                                      \
		template <>                                                                                                        \
		template <>                                                                                                        \
		Vector2<units::_ToUnit> Vector2<units::_FromUnit>::into<units::_ToUnit>(Vector2<units::_ToUnit> _) const = delete; \
		template <>                                                                                                        \
		template <>                                                                                                        \
		Vector2<units::_ToUnit> Vector2<units::_FromUnit>::into(units::_ToUnit _) const = delete;                          \
	}











namespace dim
{
	template <typename _S, typename _T>
	constexpr inline Scalar<_S, _T> operator+(Scalar<_S, _T> a, Scalar<_S, _T> b)
	{
		return Scalar<_S, _T>(a._x + b._x);
	}

	template <typename _S, typename _T>
	constexpr inline Scalar<_S, _T> operator-(Scalar<_S, _T> a, Scalar<_S, _T> b)
	{
		return Scalar<_S, _T>(a._x - b._x);
	}

	template <typename _S, typename _T, typename _ScaleT, typename = std::enable_if_t<std::is_fundamental_v<_ScaleT>>>
	constexpr inline Scalar<_S, _T> operator*(Scalar<_S, _T> value, _ScaleT scale)
	{
		return Scalar<_S, _T>(value._x * scale);
	}

	template <typename _S, typename _T, typename _ScaleT, typename = std::enable_if_t<std::is_fundamental_v<_ScaleT>>>
	constexpr inline Scalar<_S, _T> operator*(_ScaleT scale, Scalar<_S, _T> value)
	{
		return Scalar<_S, _T>(value._x * scale);
	}

	template <typename _S, typename _T, typename _ScaleT>
	constexpr inline Scalar<_S, _T> operator/(Scalar<_S, _T> value, _ScaleT scale)
	{
		return Scalar<_S, _T>(value._x / scale);
	}

	template <typename _S, typename _T>
	constexpr inline _T operator/(Scalar<_S, _T> a, Scalar<_S, _T> b)
	{
		return a._x / b._x;
	}



	template <typename _S, typename _T>
	constexpr inline Vector2<_S, _T> operator+(Vector2<_S, _T> a, Vector2<_S, _T> b)
	{
		return Vector2<_S, _T>(a._x + b._x, a._y + b._y);
	}

	template <typename _S, typename _T>
	constexpr inline Vector2<_S, _T> operator-(Vector2<_S, _T> a, Vector2<_S, _T> b)
	{
		return Vector2<_S, _T>(a._x - b._x, a._y - b._y);
	}

	template <typename _S, typename _T, typename _ScaleT>
	constexpr inline Vector2<_S, _T> operator*(Vector2<_S, _T> value, _ScaleT scale)
	{
		return Vector2<_S, _T>(value._x * scale, value._y * scale);
	}

	template <typename _S, typename _T>
	constexpr inline Vector2<_S, _T> operator*(_T scale, Vector2<_S, _T> value)
	{
		return Vector2<_S, _T>(value._x * scale, value._y * scale);
	}

	template <typename _S, typename _T, typename _ScaleT>
	constexpr inline Vector2<_S, _T> operator/(Vector2<_S, _T> value, _ScaleT scale)
	{
		return Vector2<_S, _T>(value._x / scale, value._y / scale);
	}

	template <typename _S, typename _T>
	constexpr inline Scalar<_S, _T> min(Scalar<_S, _T> a, Scalar<_S, _T> b)
	{
		if(a._x < b._x)
			return a;
		else
			return b;
	}

	template <typename _S, typename _T>
	constexpr inline Scalar<_S, _T> max(Scalar<_S, _T> a, Scalar<_S, _T> b)
	{
		if(a._x > b._x)
			return a;
		else
			return b;
	}
}





template <typename _S, typename _T>
struct zpr::print_formatter<dim::Scalar<_S, _T>>
{
	template <typename Cb>
	void print(dim::Scalar<_S, _T> x, Cb&& cb, format_args args)
	{
		detail::print_one(cb, args, x._x);
	}
};

template <typename _S, typename _T>
struct zpr::print_formatter<dim::Vector2<_S, _T>>
{
	template <typename Cb>
	void print(dim::Vector2<_S, _T> x, Cb&& cb, format_args args)
	{
		detail::print(cb, "{} {}", x._x, x._y);
	}
};
