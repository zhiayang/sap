// types.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bit>
#include <type_traits>

#include "endian.h"

enum class GlyphId : uint32_t
{
	notdef = 0
};

// super annoying
constexpr inline GlyphId operator+(GlyphId gid, size_t s)
{
	return GlyphId(static_cast<uint32_t>(gid) + s);
}

constexpr inline GlyphId operator-(GlyphId gid, size_t s)
{
	return GlyphId(static_cast<uint32_t>(gid) - s);
}

constexpr inline ssize_t operator-(GlyphId a, GlyphId b)
{
	if(a > b)
		return static_cast<uint32_t>(a) - static_cast<uint32_t>(b);
	else
		return static_cast<uint32_t>(b) - static_cast<uint32_t>(a);
}

#if 0
struct beu16_t
{
	constexpr beu16_t() = default;
	explicit beu16_t(uint16_t x) : m_value(util::convertBEU16(x)) { }

	constexpr uint16_t native() const
	{
		if constexpr(std::endian::native == std::endian::big)
			return util::convertBEU16(m_value);
		else
			return m_value;
	}

	constexpr operator uint16_t() const { return this->native(); }

	constexpr bool operator==(const beu16_t& x) const = default;
	constexpr auto operator<=>(const beu16_t& x) const = default;

private:
	uint16_t m_value;
};

struct beu32_t
{
	constexpr beu32_t() = default;
	constexpr explicit beu32_t(uint32_t x) : m_value(util::convertBEU32(x)) { }

	constexpr uint32_t native() const
	{
		if constexpr(std::endian::native == std::endian::big)
			return util::convertBEU32(m_value);
		else
			return m_value;
	}

	constexpr operator uint32_t() const { return this->native(); }

	constexpr bool operator==(const beu32_t& x) const = default;
	constexpr auto operator<=>(const beu32_t& x) const = default;

private:
	uint32_t m_value;
};

static_assert(std::is_trivial_v<beu16_t>);
static_assert(std::is_trivial_v<beu32_t>);

static_assert(sizeof(beu16_t) == sizeof(uint16_t));
static_assert(sizeof(beu32_t) == sizeof(uint32_t));
#endif


constexpr inline GlyphId operator""_gid(unsigned long long x)
{
	return static_cast<GlyphId>(x);
}

namespace zpr
{
	template <>
	struct print_formatter<GlyphId>
	{
		template <typename Cb>
		void print(GlyphId x, Cb&& cb, format_args args)
		{
			detail::print(static_cast<Cb&&>(cb), "glyph({})", static_cast<uint32_t>(x));
		}
	};

	template <>
	struct print_formatter<char32_t>
	{
		template <typename Cb>
		void print(char32_t x, Cb&& cb, format_args args)
		{
			detail::print(static_cast<Cb&&>(cb), "char32({})", static_cast<uint32_t>(x));
		}
	};
}
