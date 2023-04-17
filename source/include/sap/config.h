// config.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include <string>

namespace sap::config
{
	struct MicrotypeConfig
	{
		util::hashset<std::string> matched_fonts;

		bool enable_if_italic = false;
		std::vector<util::hashset<std::string>> required_features;

		util::hashmap<char32_t, CharacterProtrusion> protrusions;
	};

	StrErrorOr<MicrotypeConfig> parseMicrotypeConfig(zst::str_view& contents);
}
