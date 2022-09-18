// subset_cmap.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cmath>

#include "util.h"

#include "font/cff.h"
#include "font/font.h"

namespace font::cff
{
	zst::byte_buffer createCMapForCFFSubset(FontFile* file)
	{
		/*
		    i guess what we want to do is just map the cid to the gid...
		*/

		auto append16 = [](zst::byte_buffer& buf, uint16_t x) {
			buf.append_bytes(util::convertBEU16(x));
		};

		auto cff = file->cff_data;
		assert(cff != nullptr);

		zst::byte_buffer cmap {};

		append16(cmap, 0);
		append16(cmap, 2);

		auto subtable_offset = (2 + 2) + 2 * (2 + 2 + 4);

		append16(cmap, 0);
		append16(cmap, 4);
		cmap.append_bytes(util::convertBEU32(subtable_offset));

		append16(cmap, 3);
		append16(cmap, 1);
		cmap.append_bytes(util::convertBEU32(subtable_offset));

		std::map<uint16_t, uint16_t> mapping {};
		for(auto& glyph : cff->glyphs)
			mapping[glyph.gid] = glyph.cid;

		auto seg_count = cff->glyphs.size() + 1;

		// format 4
		append16(cmap, 4);
		append16(cmap, 8 * 2 + (4 * 2 * seg_count));
		append16(cmap, 0);
		append16(cmap, 2 * seg_count);

		auto search_range = 2 * (1 << (int) log2(seg_count));
		append16(cmap, search_range);
		append16(cmap, log2(search_range / 2));
		append16(cmap, 2 * seg_count - search_range);

		zst::byte_buffer start_codes {};
		zst::byte_buffer end_codes {};
		zst::byte_buffer id_deltas {};
		zst::byte_buffer id_range_offsets {};

		for(auto& [gid, cid] : mapping)
		{
			append16(start_codes, gid);
			append16(end_codes, gid + 1);
			append16(id_deltas, cid - gid);
			append16(id_range_offsets, 0);
		}

		append16(end_codes, 0xFFFF);
		append16(start_codes, 0xFFFF);
		append16(id_deltas, 1);
		append16(id_range_offsets, 0);


		cmap.append(end_codes.span());
		append16(cmap, 0);
		cmap.append(start_codes.span());
		cmap.append(id_deltas.span());
		cmap.append(id_range_offsets.span());

		return cmap;
	}
}
