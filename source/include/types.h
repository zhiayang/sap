// types.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

#include <zpr.h>

enum class GlyphId : uint32_t
{
	notdef = 0
};
enum class Codepoint : uint32_t
{
};

constexpr inline GlyphId operator""_gid(unsigned long long x)
{
	return static_cast<GlyphId>(x);
}
constexpr inline Codepoint operator""_codepoint(unsigned long long x)
{
	return static_cast<Codepoint>(x);
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
	struct print_formatter<Codepoint>
	{
		template <typename Cb>
		void print(Codepoint x, Cb&& cb, format_args args)
		{
			detail::print(static_cast<Cb&&>(cb), "codepoint({})", static_cast<uint32_t>(x));
		}
	};
}
