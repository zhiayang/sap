// misc.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

namespace font
{
	struct FontFile;

	uint8_t peek_u8(const zst::byte_span& s);
	int16_t peek_i16(const zst::byte_span& s);
	uint16_t peek_u16(const zst::byte_span& s);
	uint32_t peek_u32(const zst::byte_span& s);
	uint64_t peek_u64(const zst::byte_span& s);

	uint8_t consume_u8(zst::byte_span& s);
	uint16_t consume_u16(zst::byte_span& s);
	int16_t consume_i16(zst::byte_span& s);
	uint32_t consume_u24(zst::byte_span& s);
	uint32_t consume_u32(zst::byte_span& s);
	uint64_t consume_u64(zst::byte_span& s);

	std::string generateSubsetName(const FontFile* font);
}
