// types.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

enum class GlyphId : uint32_t
{
	notdef = 0
};

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
