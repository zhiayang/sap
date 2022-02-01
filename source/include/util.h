// util.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <utility>

#include <zst.h>

#include "types.h"

namespace util
{
	std::pair<uint8_t*, size_t> readEntireFile(const std::string& path);

	uint16_t convertBEU16(uint16_t x);
	uint32_t convertBEU32(uint32_t x);
}


namespace unicode
{
	std::string utf8FromUtf16(zst::span<uint16_t> utf16);
	std::string utf8FromUtf16BigEndianBytes(zst::byte_span bytes);

	Codepoint consumeCodepointFromUtf8(zst::byte_span& utf8);

	// high-order surrogate first.
	std::pair<uint16_t, uint16_t> codepointToSurrogatePair(Codepoint codepoint);
}
