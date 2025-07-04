// subset.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "util.h"
#include "types.h"

#include "font/truetype.h"
#include "font/font_file.h"

namespace font
{
	truetype::TTSubset FontFile::createTTSubset()
	{
		auto& tt = m_truetype_data;
		assert(tt != nullptr);

		// needs to be sorted. always insert 0.
		std::set<uint16_t> used_gids {};
		used_gids.insert(0);
		used_gids.insert(tt->glyphs[0].component_gids.begin(), tt->glyphs[0].component_gids.end());

		for(auto& gid : m_used_glyphs)
		{
			auto gid16 = util::checked_cast<uint16_t>(static_cast<uint32_t>(gid));
			auto& comps = tt->glyphs[gid16].component_gids;

			used_gids.insert(gid16);
			used_gids.insert(comps.begin(), comps.end());
		}

		truetype::TTSubset subset {};

		// the loca table must contain an entry for every glyph id in the font. since we're not
		// changing the glyph ids themselves, we must iterate over every glyph id.
		{
			bool half = tt->loca_bytes_per_entry == 2;

			zst::byte_buffer loca {};
			zst::byte_buffer glyf {};

			for(uint16_t gid = 0; gid < m_num_glyphs; gid++)
			{
				if(half)
					loca.append_bytes(util::convertBEU16(util::checked_cast<uint16_t>(glyf.size() / 2)));
				else
					loca.append_bytes(util::convertBEU32(util::checked_cast<uint32_t>(glyf.size())));

				if(used_gids.find(gid) != used_gids.end())
					glyf.append(tt->glyphs[gid].glyph_data);
			}

			subset.loca_table = std::move(loca);
			subset.glyf_table = std::move(glyf);
		}

		return subset;
	}
}
