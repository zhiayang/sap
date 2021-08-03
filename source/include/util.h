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
}


namespace unicode
{
	std::string utf8FromUtf16(zst::span<uint16_t> utf16);
	std::string utf8FromUtf16BigEndianBytes(zst::byte_span bytes);
}
