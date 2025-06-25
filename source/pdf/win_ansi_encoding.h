// win_ansi_encoding.h
// Copyright (c) 2021, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "types.h"

namespace pdf::encoding
{
	static inline uint8_t WIN_ANSI(char32_t cp)
	{
		static std::map<char32_t, uint8_t> cmap;
		if(cmap.empty())
		{
			for(char32_t i = 0; i < 128; i++)
				cmap[i] = static_cast<uint8_t>(i);

			cmap[U'\u20ac'] = 128;
			cmap[U'\u201a'] = 130;
			cmap[U'\u0192'] = 131;
			cmap[U'\u201e'] = 132;
			cmap[U'\u2026'] = 133;
			cmap[U'\u2020'] = 134;
			cmap[U'\u2021'] = 135;
			cmap[U'\u02c6'] = 136;
			cmap[U'\u2030'] = 137;
			cmap[U'\u0160'] = 138;
			cmap[U'\u2039'] = 139;
			cmap[U'\u0152'] = 140;
			cmap[U'\u017d'] = 142;
			cmap[U'\u2018'] = 145;
			cmap[U'\u2019'] = 146;
			cmap[U'\u201c'] = 147;
			cmap[U'\u201d'] = 148;
			cmap[U'\u2022'] = 149;
			cmap[U'\u2013'] = 150;
			cmap[U'\u2014'] = 151;
			cmap[U'\u02dc'] = 152;
			cmap[U'\u2122'] = 153;
			cmap[U'\u0161'] = 154;
			cmap[U'\u203a'] = 155;
			cmap[U'\u0153'] = 156;
			cmap[U'\u017e'] = 158;
			cmap[U'\u0178'] = 159;

			for(char32_t i = 0xa0; i <= 0xff; i++)
				cmap[i] = static_cast<uint8_t>(i);
		}

		return cmap[cp];
	}
}
