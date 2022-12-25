// misc.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/misc.h"

namespace pdf
{
	std::string encodeStringLiteral(zst::str_view sv)
	{
		std::string ret;
		ret.reserve(sv.size() * 2 + 2);

		ret += "<";
		for(int8_t c : sv)
			ret += zpr::sprint("{02x}", static_cast<uint8_t>(c));

		ret += ">";
		return ret;
	}
}
