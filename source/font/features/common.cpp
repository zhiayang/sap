// common.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "util.h"
#include "font/font.h"
#include "font/features.h"

namespace font
{
	std::vector<TaggedTable> parseTaggedList(FontFile* font, zst::byte_span list)
	{
		auto table_start = list;

		std::vector<TaggedTable> ret;

		auto num = consume_u16(list);
		for(size_t i = 0; i < num; i++)
		{
			auto tag = Tag(consume_u32(list));
			auto ofs = consume_u16(list);

			ret.push_back({ tag, static_cast<size_t>(ofs + (table_start.data() - font->file_bytes)) });
		}

		return ret;
	}
}
