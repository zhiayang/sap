// tester.h
// Copyright (c) 2024, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "defs.h"

namespace test
{
	struct Context
	{
		size_t passed;
		size_t failed;
		size_t skipped;
	};

	void test_parser(Context& ctx, const stdfs::path& test_dir);








	// helpers and stuff
	template <typename Iter, typename Predicate>
	inline void find_files_helper(std::vector<stdfs::path>& list, const stdfs::path& dir, Predicate&& pred)
	{
		// i guess this is not an error...?
		if(!stdfs::is_directory(dir))
			return;

		auto iter = Iter(dir);
		for(auto& ent : iter)
		{
			if((ent.is_regular_file() || ent.is_symlink()) && pred(ent))
				list.push_back(ent.path());
		}
	}

	/*
	    Search for files in the given directory (non-recursively), returning a list of paths
	    that match the given predicate. Note that the predicate should accept a `stdfs::directory_entry`,
	    *NOT* a `stdfs::path`.
	*/
	template <typename Predicate>
	inline std::vector<stdfs::path> find_files(const stdfs::path& dir, Predicate&& pred)
	{
		std::vector<stdfs::path> ret {};
		find_files_helper<stdfs::directory_iterator>(ret, dir, pred);
		return ret;
	}

	/*
	    Same as `find_files`, but recursively traverses directories.
	*/
	template <typename Predicate>
	inline std::vector<stdfs::path> find_files_recursively(const stdfs::path& dir, Predicate&& pred)
	{
		std::vector<stdfs::path> ret {};
		find_files_helper<stdfs::recursive_directory_iterator>(ret, dir, pred);
		return ret;
	}

	/*
	    Same as `find_files`, but in multiple (distinct) directories at once.
	*/
	template <typename Predicate>
	inline std::vector<stdfs::path> find_files(const std::vector<stdfs::path>& dirs, Predicate&& pred)
	{
		std::vector<stdfs::path> ret {};
		for(auto& dir : dirs)
			find_files_helper<stdfs::directory_iterator>(ret, dir, pred);
		return ret;
	}

	/*
	    Same as `find_files_recursively`, but in multiple (distinct) directories at once.
	*/
	template <typename Predicate>
	inline std::vector<stdfs::path> find_files_recursively(const std::vector<stdfs::path>& dirs, Predicate&& pred)
	{
		std::vector<stdfs::path> ret {};
		for(auto& dir : dirs)
			find_files_helper<stdfs::recursive_directory_iterator>(ret, dir, pred);
		return ret;
	}

	inline std::vector<stdfs::path> find_files_ext(const stdfs::path& dir, std::string_view ext)
	{
		return find_files(dir, [&ext](auto ent) -> bool { return ent.path().extension() == ext; });
	}

	inline std::vector<stdfs::path> find_files_ext_recursively(const stdfs::path& dir, std::string_view ext)
	{
		return find_files_recursively(dir, [&ext](auto ent) -> bool { return ent.path().extension() == ext; });
	}
}
