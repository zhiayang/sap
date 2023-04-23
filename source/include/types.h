// types.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bit>
#include <cstdint>
#include <type_traits>

#include <zpr.h>
#include <zst/zst.h>

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

namespace sap
{
	enum class ProcessingPhase
	{
		Start,
		Preamble,
		Layout,
		Position,
		PostLayout,
		Finalise,
		Render,
	};


	struct OwnedImageBitmap;
	struct ImageBitmap
	{
		size_t pixel_width;
		size_t pixel_height;
		size_t bits_per_pixel;

		zst::byte_span rgb {};
		zst::byte_span alpha {};

		bool haveAlpha() const { return not alpha.empty(); }
		OwnedImageBitmap clone() const;
	};

	struct OwnedImageBitmap
	{
		size_t pixel_width;
		size_t pixel_height;
		size_t bits_per_pixel;

		zst::unique_span<uint8_t[]> rgb {};
		zst::unique_span<uint8_t[]> alpha {};

		bool haveAlpha() const { return not alpha.empty(); }
		OwnedImageBitmap clone() const;

		ImageBitmap span() const
		{
			return ImageBitmap {
				.pixel_width = pixel_width,
				.pixel_height = pixel_height,
				.bits_per_pixel = bits_per_pixel,
				.rgb = rgb.span(),
				.alpha = alpha.span(),
			};
		}
	};

	struct CharacterProtrusion
	{
		double left;
		double right;
	};


	template <typename T>
	struct Uninitialised
	{
		static_assert(std::is_trivially_destructible_v<T>);

		Uninitialised() { }
		~Uninitialised() { }

		Uninitialised(Uninitialised&&) = default;
		Uninitialised(const Uninitialised&) = default;
		Uninitialised& operator=(Uninitialised&&) = default;
		Uninitialised& operator=(const Uninitialised&) = default;

		using value_type = T;

		template <typename... Args>
		void init(Args&&... args)
		{
			new(&value) T(static_cast<Args&&>(args)...);
		}

		void set(T val) { new(&value) T(static_cast<T&&>(val)); }
		void destroy() { value.~T(); };

		const T& operator*() const { return this->value; }
		T& operator*() { return this->value; }

		union alignas(alignof(T))
		{
			uint8_t buf[sizeof(T)];
			T value;
		};
	};
}

template <>
struct std::hash<GlyphId>
{
	size_t operator()(const GlyphId& gid) const { return std::hash<size_t>()(static_cast<size_t>(gid)); }
};

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
			detail::print(static_cast<Cb&&>(cb), "char32({x})", static_cast<uint32_t>(x));
		}
	};
}
