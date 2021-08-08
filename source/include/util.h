// util.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <string>
#include <utility>

#include <zst.h>

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

	uint32_t consumeCodepointFromUtf8(zst::byte_span& utf8);

	// high-order surrogate first.
	std::pair<uint16_t, uint16_t> codepointToSurrogatePair(uint32_t codepoint);
}
