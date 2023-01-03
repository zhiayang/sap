// endian.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bit>
#include <concepts>
#include <type_traits>

namespace util
{
	inline uint16_t convertBEU16(uint16_t x)
	{
		if constexpr(std::endian::native == std::endian::big)
		{
			// Untested cause who has big endian anyway
			return x;
		}
		else
		{
			return zst::byteswap(x);
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
			return zst::byteswap(x);
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
			return zst::byteswap(x);
		}
	}

}
