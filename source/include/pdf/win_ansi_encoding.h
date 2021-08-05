// win_ansi_encoding.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <map>

namespace pdf::encoding
{
	static inline uint8_t WIN_ANSI(uint32_t cp)
	{
		static std::map<uint32_t, uint8_t> cmap;
		if(cmap.empty())
		{
			for(int i = 0; i < 128; i++)
				cmap[i] = i;

			cmap[0x20ac] = 128;
			cmap[0x201a] = 130;
			cmap[0x0192] = 131;
			cmap[0x201e] = 132;
			cmap[0x2026] = 133;
			cmap[0x2020] = 134;
			cmap[0x2021] = 135;
			cmap[0x02c6] = 136;
			cmap[0x2030] = 137;
			cmap[0x0160] = 138;
			cmap[0x2039] = 139;
			cmap[0x0152] = 140;
			cmap[0x017d] = 142;
			cmap[0x2018] = 145;
			cmap[0x2019] = 146;
			cmap[0x201c] = 147;
			cmap[0x201d] = 148;
			cmap[0x2022] = 149;
			cmap[0x2013] = 150;
			cmap[0x2014] = 151;
			cmap[0x02dc] = 152;
			cmap[0x2122] = 153;
			cmap[0x0161] = 154;
			cmap[0x203a] = 155;
			cmap[0x0153] = 156;
			cmap[0x017e] = 158;
			cmap[0x0178] = 159;

			for(int i = 0xa0; i <= 0xff; i++)
				cmap[i] = i;
		}

		return cmap[cp];
	}
}
