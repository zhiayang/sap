// paths.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <unordered_set>

#include "sap/config.h"

namespace sap::paths
{
	static std::vector<std::string> g_include_paths;
	static std::vector<std::string> g_library_paths;

	const std::vector<std::string>& librarySearchPaths()
	{
		return g_library_paths;
	}

	void addIncludeSearchPath(std::string path)
	{
		g_include_paths.push_back(std::move(path));
	}

	void addLibrarySearchPath(std::string path)
	{
		g_library_paths.push_back(std::move(path));
	}

	static ErrorOr<std::string>
	resolve_path(const Location& loc, zst::str_view _path, const std::vector<std::string>& search)
	{
		// first, try to resolve it relative to the current directory
		auto cwd = stdfs::current_path();
		auto path = stdfs::path(_path.sv());

		if(auto p = cwd / path; stdfs::exists(p))
			return Ok(p.string());

		for(auto it = search.rbegin(); it != search.rend(); ++it)
		{
			if(auto p = stdfs::path(*it) / path; stdfs::exists(p))
				return Ok(p.string());
		}

		return ErrMsg(loc, "file '{}' could not be found", _path);
	}

	ErrorOr<std::string> resolveInclude(const Location& loc, zst::str_view path)
	{
		return resolve_path(loc, path, g_include_paths);
	}

	ErrorOr<std::string> resolveLibrary(const Location& loc, zst::str_view path)
	{
		return resolve_path(loc, path, g_library_paths);
	}
}
