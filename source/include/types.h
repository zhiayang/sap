// types.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bit>
#include <type_traits>

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
