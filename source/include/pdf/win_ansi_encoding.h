// win_ansi_encoding.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>

#include "types.h"

namespace pdf::encoding
{
	static inline uint8_t WIN_ANSI(Codepoint cp)
	{
		static std::map<Codepoint, uint8_t> cmap;
		if(cmap.empty())
		{
			for(uint32_t i = 0; i < 128; i++)
				cmap[Codepoint { i }] = i;

			cmap[0x20ac_codepoint] = 128;
			cmap[0x201a_codepoint] = 130;
			cmap[0x0192_codepoint] = 131;
			cmap[0x201e_codepoint] = 132;
			cmap[0x2026_codepoint] = 133;
			cmap[0x2020_codepoint] = 134;
			cmap[0x2021_codepoint] = 135;
			cmap[0x02c6_codepoint] = 136;
			cmap[0x2030_codepoint] = 137;
			cmap[0x0160_codepoint] = 138;
			cmap[0x2039_codepoint] = 139;
			cmap[0x0152_codepoint] = 140;
			cmap[0x017d_codepoint] = 142;
			cmap[0x2018_codepoint] = 145;
			cmap[0x2019_codepoint] = 146;
			cmap[0x201c_codepoint] = 147;
			cmap[0x201d_codepoint] = 148;
			cmap[0x2022_codepoint] = 149;
			cmap[0x2013_codepoint] = 150;
			cmap[0x2014_codepoint] = 151;
			cmap[0x02dc_codepoint] = 152;
			cmap[0x2122_codepoint] = 153;
			cmap[0x0161_codepoint] = 154;
			cmap[0x203a_codepoint] = 155;
			cmap[0x0153_codepoint] = 156;
			cmap[0x017e_codepoint] = 158;
			cmap[0x0178_codepoint] = 159;

			for(uint32_t i = 0xa0; i <= 0xff; i++)
				cmap[Codepoint { i }] = i;
		}

		return cmap[cp];
	}
}
