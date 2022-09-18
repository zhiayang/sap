// subset.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"

#include "font/font.h"
#include "font/truetype.h"

namespace font::truetype
{
	TTSubset createTTSubset(FontFile* font, const std::unordered_set<GlyphId>& used_glyphs)
	{
		auto tt = font->truetype_data;
		assert(tt != nullptr);

		// needs to be sorted. always insert 0.
		std::set<uint16_t> used_gids {};
		used_gids.insert(0);
		used_gids.insert(tt->glyphs[0].component_gids.begin(), tt->glyphs[0].component_gids.end());

		for(auto& gid : used_glyphs)
		{
			auto gid32 = static_cast<uint32_t>(gid);
			auto& comps = tt->glyphs[gid32].component_gids;

			used_gids.insert(gid32);
			used_gids.insert(comps.begin(), comps.end());
		}

		TTSubset subset {};

		// the loca table must contain an entry for every glyph id in the font. since we're not
		// changing the glyph ids themselves, we must iterate over every glyph id.
		{
			bool half = tt->loca_bytes_per_entry == 2;

			zst::byte_buffer loca {};
			zst::byte_buffer glyf {};

			for(size_t gid = 0; gid < font->num_glyphs; gid++)
			{
				if(half)
					loca.append_bytes(util::convertBEU16(glyf.size() / 2));
				else
					loca.append_bytes(util::convertBEU32(glyf.size()));

				if(used_gids.find(gid) != used_gids.end())
					glyf.append(tt->glyphs[gid].glyph_data);
			}

			subset.loca_table = std::move(loca);
			subset.glyf_table = std::move(glyf);
		}

		return subset;
	}
}
