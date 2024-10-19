// config.h
// Copyright (c) 2022, yuki / zhiayang
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

namespace sap::paths
{
	void addIncludeSearchPath(std::string path);
	void addLibrarySearchPath(std::string path);

	const std::vector<std::string>& librarySearchPaths();

	ErrorOr<std::string> resolveInclude(const Location& loc, zst::str_view path);
	ErrorOr<std::string> resolveLibrary(const Location& loc, zst::str_view path);
}
