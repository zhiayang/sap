// subset.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "font/cff.h"
#include "font/font.h"

namespace font::cff
{
	zst::byte_buffer createCFFSubset(FontFile* file, const std::map<uint32_t, GlyphMetrics>& used_glyphs)
	{
		auto cff = file->cff_data;
		assert(cff != nullptr);

		zst::byte_buffer buffer {};
		buffer.append(cff->bytes);

		return buffer;
	}
}
