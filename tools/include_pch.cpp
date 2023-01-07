// include_pch.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>

#include <string>
#include <fstream>
#include <filesystem>

#include "../external/zst.h"
#include "../external/zpr.h"

namespace stdfs = std::filesystem;


template <typename Callable>
static void iterate_lines(std::string_view contents, Callable on_line)
{
	auto line_it = contents.begin();
	auto it = contents.begin();
	size_t len = 0;
	for(auto c : contents)
	{
		++it;
		if(c == '\n')
		{
			on_line(std::string_view(line_it, len));
			line_it = it;
			len = 0;
		}
		else
		{
			++len;
		}
	}

	if(len > 0)
	{
		on_line(std::string_view(line_it, len));
	}
}

static bool header_mtimes_indicate_that_we_need_to_rebuild(std::string_view output_name)
{
	if(not stdfs::exists(output_name))
		return true;

	auto file = std::ifstream(output_name, std::ios::in);

	std::string foo;
	while(std::getline(file, foo))
	{
		auto line = zst::str_view(foo);
		auto i = line.find("//");
		assert(i != (size_t) -1);

		auto front = line.take(i);
		front.remove_prefix(1 + front.find('"'));
		front.remove_suffix(front.size() - front.rfind('"'));

		auto back = line.drop(1 + line.find('='));

		if(not stdfs::exists(front.sv()))
			return true;

		auto actual_mtime = std::chrono::duration_cast<
		    std::chrono::microseconds>(stdfs::last_write_time(front.sv()).time_since_epoch())
		                        .count();

		auto alleged_mtime = static_cast<int64_t>(std::stod(back.str()) * 1'000'000);

		if(actual_mtime != alleged_mtime)
			return true;
	}

	return false;
}


int main(int argc, char** argv)
{
	if(argc < 4)
	{
		fprintf(stderr, "usage: ./include_pch <cxx> <fallback.h> <output_basename>");
		exit(1);
	}

	/*
	    the purpose of this script is to switch between
	     `- include precompile.h` -- when the pch and / or pch.o aren't ready yet
	     `- include - pch %.gch` -- when both the pch and pch.o are ready, and we're on clang
	     `- include %.h`        -- on gcc
	*/
	auto cxx = std::string(argv[1]);
	auto fallback_hdr = std::string(argv[2]);

	auto output_name = std::string(argv[3]);

	bool is_clang = false;
	bool fallback = not stdfs::exists(output_name + ".gch");

	if(cxx.find("clang") != std::string_view::npos)
		is_clang = true, fallback |= (not stdfs::exists(output_name + ".gch.o"));

	fallback |= header_mtimes_indicate_that_we_need_to_rebuild(output_name + ".inc");

	if(fallback)
	{
		printf("-include %s", fallback_hdr.c_str());
	}
	else
	{
		if(is_clang)
			printf("-include-pch %s.gch", output_name.c_str());
		else
			printf("-include %s.h", output_name.c_str());
	}

	fflush(stdout);
}
