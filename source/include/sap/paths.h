// paths.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "location.h"

namespace sap::paths
{
	void addIncludeSearchPath(std::string path);
	void addLibrarySearchPath(std::string path);

	const std::vector<std::string>& librarySearchPaths();

	ErrorOr<std::string> resolveInclude(const Location& loc, zst::str_view path);
	ErrorOr<std::string> resolveLibrary(const Location& loc, zst::str_view path);
}
