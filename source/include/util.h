// util.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bit>
#include <memory>
#include <string>
#include <numeric>
#include <utility>
#include <assert.h>
#include <concepts>
#include <type_traits>

#include <zst.h>

#include "types.h"

namespace util
{
	template <typename It>
	struct subrange
	{
		using value_type = typename std::iterator_traits<It>::value_type;
		It m_begin;
		It m_end;
		subrange(It begin, It end) : m_begin(begin), m_end(end) { }
		It begin() const { return m_begin; }
		It end() const { return m_end; }
	};

	std::pair<uint8_t*, size_t> readEntireFile(const std::string& path);

	uint16_t convertBEU16(uint16_t x);
	uint32_t convertBEU32(uint32_t x);

	template <std::integral From>
	std::make_unsigned_t<From> to_unsigned(From f)
	{
		return static_cast<std::make_unsigned_t<From>>(f);
	}

	template <std::integral To, std::integral From>
	requires requires()
	{
		requires sizeof(To) < sizeof(From);
	}
	To checked_cast(From f)
	{
		if constexpr(std::signed_integral<To> && std::unsigned_integral<From>)
		{
			assert(f <= static_cast<From>(std::numeric_limits<To>::max()));
		}
		else if constexpr(std::unsigned_integral<To> && std::signed_integral<From>)
		{
			assert(f >= 0);
			assert(f <= static_cast<From>(std::numeric_limits<To>::max()));
		}
		else
		{
			// same signedness
			assert(f >= static_cast<From>(std::numeric_limits<To>::min()));
			assert(f <= static_cast<From>(std::numeric_limits<To>::max()));
		}
		return static_cast<To>(f);
	}

	template <std::integral To, std::integral From>
	requires requires()
	{
		requires sizeof(To) == sizeof(From);
	}
	To checked_cast(From f)
	{
		if constexpr(std::signed_integral<To> && std::unsigned_integral<From>)
		{
			assert(f <= static_cast<From>(std::numeric_limits<To>::max()));
		}
		else if constexpr(std::unsigned_integral<To> && std::signed_integral<From>)
		{
			assert(f >= 0);
		}
		else
		{
			// same signedness and same size, no checks required
		}
		return static_cast<To>(f);
	}

	template <std::integral To, std::integral From>
	requires requires()
	{
		requires sizeof(To) > sizeof(From);
	}
	To checked_cast(From f)
	{
		if constexpr(std::signed_integral<To> && std::unsigned_integral<From>)
		{
			// To bigger signed, no check required
		}
		else if constexpr(std::unsigned_integral<To> && std::signed_integral<From>)
		{
			assert(f >= 0);
		}
		else
		{
			// same signedness and casting to bigger size, no check required
		}
		return static_cast<To>(f);
	}

	template <std::signed_integral T>
	inline T byteswap(T x)
	{
		return byteswap<std::make_unsigned_t<T>>(static_cast<std::make_unsigned_t<T>>(x));
	}

	inline uint16_t byteswap(uint16_t x)
	{
#if(_MSC_VER)
		return _byteswap_ushort(value);
#else
		// Compiles to bswap since gcc 4.5.3 and clang 3.4.1
		return (uint16_t) (((uint16_t) (x & 0x00ff) << 8) | ((uint16_t) (x & 0xff00) >> 8));
#endif
	}

	inline uint32_t byteswap(uint32_t x)
	{
#if(_MSC_VER)
		return _byteswap_ulong(value);
#else
		// Compiles to bswap since gcc 4.5.3 and clang 3.4.1
		return ((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24);
#endif
	}

	inline uint64_t byteswap(uint64_t value)
	{
#if(_MSC_VER)
		return _byteswap_uint64(value);
#else
		// Compiles to bswap since gcc 4.5.3 and clang 3.4.1
		return ((value & 0xFF00000000000000u) >> 56u) | ((value & 0x00FF000000000000u) >> 40u)
		     | ((value & 0x0000FF0000000000u) >> 24u) | ((value & 0x000000FF00000000u) >> 8u)
		     | ((value & 0x00000000FF000000u) << 8u) | ((value & 0x0000000000FF0000u) << 24u)
		     | ((value & 0x000000000000FF00u) << 40u) | ((value & 0x00000000000000FFu) << 56u);
#endif
	}

	inline uint16_t convertBEU16(uint16_t x)
	{
		if constexpr(std::endian::native == std::endian::big)
		{
			// Untested cause who has big endian anyway
			return x;
		}
		else
		{
			return byteswap(x);
		}
	}

	inline uint32_t convertBEU32(uint32_t x)
	{
		if constexpr(std::endian::native == std::endian::big)
		{
			// Untested cause who has big endian anyway
			return x;
		}
		else
		{
			return byteswap(x);
		}
	}

	inline uint64_t convertBEU64(uint64_t x)
	{
		if constexpr(std::endian::native == std::endian::big)
		{
			// Untested cause who has big endian anyway
			return x;
		}
		else
		{
			return byteswap(x);
		}
	}



	// Convert type of `shared_ptr`, via `dynamic_cast`
	// This exists because libstdc++ is a dum dum
	template <typename To, typename From>
	inline std::shared_ptr<To> dynamic_pointer_cast(const std::shared_ptr<From>& from) //
	    noexcept requires std::derived_from<To, From>
	{
		if(auto* to = dynamic_cast<typename std::shared_ptr<To>::element_type*>(from.get()))
			return std::shared_ptr<To>(from, to);
		return std::shared_ptr<To>();
	}

}


namespace unicode
{
	std::string utf8FromUtf16(zst::span<uint16_t> utf16);
	std::string utf8FromUtf16BigEndianBytes(zst::byte_span bytes);

	std::string utf8FromCodepoint(char32_t cp);

	char32_t consumeCodepointFromUtf8(zst::byte_span& utf8);

	// high-order surrogate first.
	std::pair<uint16_t, uint16_t> codepointToSurrogatePair(char32_t codepoint);
}
