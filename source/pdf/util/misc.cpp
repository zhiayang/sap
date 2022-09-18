// misc.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/misc.h"

namespace pdf
{
	std::string encodeStringLiteral(zst::str_view sv)
	{
#if 0
		std::string ret; ret.reserve(sv.size());
		ret += "(";
		for(char c : sv)
		{
			if(32 <= c && c <= 126)
			{
				switch(c)
				{
					case '(':
					case ')':
					case '\\':
						ret += "\\";
						ret += c;
						break;

					default:
						ret += c;
						break;
				}
			}
			else
			{
				ret += zpr::sprint("#{02x}", (uint8_t) c);
			}
		}

		return ret + ")";
#else
		std::string ret;
		ret.reserve(sv.size() * 2 + 2);

		ret += "<";
		for(uint8_t c : sv)
			ret += zpr::sprint("{02x}", c);

		ret += ">";
		return ret;
#endif
	}
}
