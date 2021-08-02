// misc.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "pdf/misc.h"

namespace pdf
{
	std::string encodeStringLiteral(zst::str_view sv)
	{
		std::string ret; ret.reserve(sv.size());
		ret += "(";
		for(char c : sv)
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

		return ret + ")";
	}
}
