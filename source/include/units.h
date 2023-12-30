// units.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <zpr.h>

namespace dim
{
	namespace units
	{
		struct millimetre; // common units are integrated into Scalar
	}

	/*
	    Both `Scalar` and `Vector` operate on the same principle. The first template parameter
	    is simply a tag, so that we do not (and cannot) confuse / interoperate values using
	    different base units.

	    For instance, in the PDF layer, we might want to operate with typographic units (72 units = 1 inch),
	    but everywhere else we probably want to stick to mm or some other metric unit. However, we do define
	    the notion of a "elementary" unit, such that all derivative units must be scaled in terms of it.

	    This allows (explicit) conversions between different measurement systems.
	*/
	template <typename, typename = double>
	struct Scalar;

	template <typename, typename _CoordSystem, typename = double>
	struct Vector2;

	namespace impl
	{
		template <typename>
		struct is_scalar : std::false_type
		{
		};

		template <typename _S, typename _T>
		struct is_scalar<Scalar<_S, _T>> : std::true_type
		{
		};

		template <typename>
		struct is_vector2 : std::false_type
		{
		};

		template <typename _S, typename _C, typename _T>
		struct is_vector2<Vector2<_S, _C, _T>> : std::true_type
		{
		};

		template <typename _FromUnit, typename _ToUnit>
		struct can_convert_units : std::false_type
		{
		};

		template <typename _U>
		struct can_convert_units<_U, _U> : std::true_type
		{
		};

		template <typename _FromCoordSys, typename _ToCoordSys>
		struct can_convert_coord_systems : std::false_type
		{
		};

		template <typename _U>
		struct can_convert_coord_systems<_U, _U> : std::true_type
		{
		};

		template <typename _System, typename _Type>
		struct ScalarConverter
		{
			using value_type = _Type;
			using unit_system = _System;

			_Type value;

			ScalarConverter operator-() const { return ScalarConverter { -value }; }
		};

		template <typename _System, typename _CoordSystem, typename _Type>
		struct Vector2Converter
		{
			using value_type = _Type;
			using unit_system = _System;
			using coord_system = _CoordSystem;

			_Type value1;
			_Type value2;

			Vector2Converter operator-() const { return Vector2Converter { -value1, -value2 }; }
		};
	}

	template <typename _System, typename _Type>
	struct Scalar
	{
		using value_type = _Type;
		using unit_system = _System;
		using self_type = Scalar<_System, _Type>;
		using unit_tag_type = typename _System::Tag;

		static constexpr auto scale_factor = _System::scale_factor;
		static_assert(scale_factor != 0);

		constexpr Scalar() : _x(0) { }

		template <std::convertible_to<value_type> T>
		constexpr explicit Scalar(T x) : _x(x)
		{
		}

		constexpr Scalar(self_type&&) = default;
		constexpr Scalar(const self_type&) = default;
		constexpr self_type& operator=(self_type&&) = default;
		constexpr self_type& operator=(const self_type&) = default;

		constexpr Scalar(std::nullptr_t) : _x(0) { }
		constexpr Scalar& operator=(std::nullptr_t)
		{
			_x = 0;
			return *this;
		}

		constexpr bool iszero() const { return this->_x == 0; }
		constexpr bool nonzero() const { return this->_x != 0; }

		constexpr value_type& value() { return this->_x; }
		constexpr value_type value() const { return this->_x; }

		constexpr self_type abs() const { return self_type(_x < 0 ? -_x : _x); }
		constexpr value_type mm() const { return this->into<dim::units::millimetre>().value(); }

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


		template <typename _Scalar>
		constexpr auto into() const
		    requires((impl::is_scalar<_Scalar>::value)
		             && (impl::can_convert_units<unit_tag_type, typename _Scalar::unit_tag_type>::value))
		{
			using Ret = Scalar<typename _Scalar::unit_system, typename _Scalar::value_type>;
			return Ret((this->_x * scale_factor) / _Scalar::unit_system::scale_factor);
		}

		template <typename _Target>
		constexpr Scalar<_Target> into() const
		    requires(impl::can_convert_units<unit_tag_type, typename _Target::Tag>::value)
		{
			return Scalar<_Target>((this->_x * scale_factor) / _Target::scale_factor);
		}


		// converting stuff
		constexpr auto into() const { return impl::ScalarConverter<_System, _Type> { _x }; }

		template <typename _FromSys, typename _FromType>
		constexpr Scalar(impl::ScalarConverter<_FromSys, _FromType> conv)
		    requires(impl::can_convert_units<typename _FromSys::Tag, unit_tag_type>::value)
		{
			this->_x = (conv.value * _FromSys::scale_factor) / scale_factor;
		}

		value_type _x;
	};

	template <typename _System, typename _CoordSystem, typename _Type>
	struct Vector2
	{
		using value_type = _Type;
		using unit_system = _System;
		using scalar_type = Scalar<_System, _Type>;
		using self_type = Vector2<_System, _CoordSystem, _Type>;
		using coord_system = _CoordSystem;
		using unit_tag_type = typename _System::Tag;

		static constexpr auto scale_factor = _System::scale_factor;
		static_assert(scale_factor != 0);

		constexpr Vector2() : _x(static_cast<_Type>(0)), _y(static_cast<_Type>(0)) { }
		constexpr Vector2(value_type x, value_type y) : _x(scalar_type(x)), _y(scalar_type(y)) { }
		constexpr Vector2(scalar_type x, scalar_type y) : _x(x), _y(y) { }

		constexpr Vector2(self_type&&) = default;
		constexpr Vector2(const self_type&) = default;
		constexpr self_type& operator=(self_type&&) = default;
		constexpr self_type& operator=(const self_type&) = default;

		constexpr bool iszero() const { return this->_x == 0 && this->_y == 0; }
		constexpr bool nonzero() const { return not this->iszero(); }

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


		template <typename _Vector>
		constexpr _Vector into() const
		    requires(                                                                                   //
		        (impl::is_vector2<_Vector>::value)                                                      //
		        && impl::can_convert_coord_systems<coord_system, typename _Vector::coord_system>::value //
		        && impl::can_convert_units<unit_tag_type, typename _Vector::unit_tag_type>::value       //
		    )
		{
			using _S = typename _Vector::unit_system;
			using _C = typename _Vector::coord_system;
			using _T = typename _Vector::value_type;

			return Vector2<_S, _C, _T>(((this->_x * scale_factor) / _S::scale_factor)._x,
			    ((this->_y * scale_factor) / _S::scale_factor)._x);
		}

		template <typename _Target>
		constexpr Vector2<_Target, coord_system> into() const
		    requires(impl::can_convert_units<unit_tag_type, typename _Target::Tag>::value)
		{
			return Vector2<_Target, coord_system>((this->_x * scale_factor) / _Target::scale_factor,
			    (this->_y * scale_factor) / _Target::scale_factor);
		}


		// converting stuff
		constexpr auto into() const { return impl::Vector2Converter<_System, _CoordSystem, _Type> { _x._x, _y._x }; }

		template <typename _FromSys, typename _FromCoordSystem, typename _FromType>
		constexpr Vector2(impl::Vector2Converter<_FromSys, _FromCoordSystem, _FromType> conv)
		    requires( //
		        (impl::can_convert_units<typename _FromSys::Tag, unit_tag_type>::value)
		        && (impl::can_convert_coord_systems<_FromCoordSystem, coord_system>::value))
		{
			this->_x = scalar_type(conv.value1 * _FromSys::scale_factor) / scale_factor;
			this->_y = scalar_type(conv.value2 * _FromSys::scale_factor) / scale_factor;
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
			using Tag = void;
		};
	}
}

/*
    due to various reasons, all units must be defined in the global namespace. use
    type aliases to bring them into an inner namespace. this also means that the names
    cannot collide.
*/
#define DEFINE_UNIT(_unit_name, _SF, _Tag)              \
	namespace dim::units                                \
	{                                                   \
		struct _unit_name                               \
		{                                               \
			static constexpr double scale_factor = _SF; \
			using Tag = _Tag;                           \
		};                                              \
	}

#define DEFINE_UNIT_IN_NAMESPACE(_unit_name, _SF, _Tag, _Namespace, _Name_in_namespace) \
	DEFINE_UNIT(_unit_name, _SF, _Tag)                                                  \
	namespace _Namespace                                                                \
	{                                                                                   \
		using _Name_in_namespace = ::dim::units::_unit_name;                            \
	}

/*
    all other units should be in terms of the base unit; so we have
    1mm = 1bu, 1cm = 10bu, etc.
*/
struct BASE_COORD_SYSTEM;
DEFINE_UNIT(millimetre, 1.0, void);
DEFINE_UNIT(centimetre, 10.0, void);

// 72 pdf user units == 1 inch == 25.4 mm
DEFINE_UNIT(pdf_user_unit, (25.4 / 72.0), void);

namespace dim::units
{
	using mm = millimetre;
	using cm = centimetre;
}

namespace dim
{
	constexpr inline auto mm(double x)
	{
		return Scalar<dim::units::mm>(x);
	}

	constexpr inline auto mm(double x, double y)
	{
		return Vector2<dim::units::mm, BASE_COORD_SYSTEM>(x, y);
	}


	constexpr inline auto cm(double x)
	{
		return Scalar<dim::units::cm>(x);
	}

	constexpr inline auto cm(double x, double y)
	{
		return Vector2<dim::units::cm, BASE_COORD_SYSTEM>(x, y);
	}
}

#define MAKE_UNITS_COMPATIBLE(_FromUnit, _ToUnit)                     \
	namespace dim::impl                                               \
	{                                                                 \
		template <>                                                   \
		struct can_convert_units<_FromUnit, _ToUnit> : std::true_type \
		{                                                             \
		};                                                            \
	}

#define MAKE_COORD_SYSTEMS_COMPATIBLE(_FromCS, _ToCS)                     \
	namespace dim::impl                                                   \
	{                                                                     \
		template <>                                                       \
		struct can_convert_coord_systems<_FromCS, _ToCS> : std::true_type \
		{                                                                 \
		};                                                                \
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



	template <typename _S, typename _C, typename _T>
	constexpr inline Vector2<_S, _C, _T> operator+(Vector2<_S, _C, _T> a, Vector2<_S, _C, _T> b)
	{
		return Vector2<_S, _C, _T>(a._x + b._x, a._y + b._y);
	}

	template <typename _S, typename _C, typename _T>
	constexpr inline Vector2<_S, _C, _T> operator-(Vector2<_S, _C, _T> a, Vector2<_S, _C, _T> b)
	{
		return Vector2<_S, _C, _T>(a._x - b._x, a._y - b._y);
	}

	template <typename _S, typename _C, typename _T, typename _ScaleT>
	constexpr inline Vector2<_S, _C, _T> operator*(Vector2<_S, _C, _T> value, _ScaleT scale)
	{
		return Vector2<_S, _C, _T>(value._x * scale, value._y * scale);
	}

	template <typename _S, typename _C, typename _T>
	constexpr inline Vector2<_S, _C, _T> operator*(_T scale, Vector2<_S, _C, _T> value)
	{
		return Vector2<_S, _C, _T>(value._x * scale, value._y * scale);
	}

	template <typename _S, typename _C, typename _T, typename _ScaleT>
	constexpr inline Vector2<_S, _C, _T> operator/(Vector2<_S, _C, _T> value, _ScaleT scale)
	{
		return Vector2<_S, _C, _T>(value._x / scale, value._y / scale);
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
