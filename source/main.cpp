// main.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/font.h"   //
#include "pdf/writer.h" // for Writer

#include "sap/frontend.h" // for parse

#include "tree/document.h"
#include "tree/container.h"

#include "layout/document.h" // for Document

#include "interp/interp.h" // for Interpreter

#if 0
CLEANUP:
- maybe wrapper around (const Style*)

TODO:
scripting:
- dereferencing optionals might need to clone them (which is weird)

layout:
- em/en dashes
- smart quotes
- microtype-like stuff (hanging punctuation)
- dijkstra linebreaking might accidentally make an extra page;
	- when rendering, use some mechanism (eg. proxy object) to only make the page if
		someone tried to actually put stuff in it (or access it)
- control comma spacing stretch as well (similar to sentence ending)

#endif

bool sap::compile(zst::str_view filename)
{
	auto interp = interp::Interpreter();
	auto file = interp.loadFile(filename);

	auto document = frontend::parse(filename, file.chars());
	if(document.is_err())
		return document.error().display(), false;

	auto layout_doc = document.unwrap().layout(&interp);
	if(layout_doc.is_err())
		return layout_doc.error().display(), false;

	interp.setCurrentPhase(ProcessingPhase::Render);
	auto out_path = std::filesystem::path(filename.str()).replace_extension(".pdf");
	auto writer = pdf::Writer(out_path.string());
	layout_doc.unwrap()->write(&writer);
	writer.close();

	return true;
}

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		zpr::println("Usage: {} [options...] <input file>", argv[0]);
		return 1;
	}

	bool is_watching = false;
	bool no_more_opts = false;

	int last = 1;
	for(; last < argc && (not no_more_opts); last++)
	{
		auto sv = zst::str_view(argv[last]);
		if(sv == "--")
		{
			no_more_opts = true;
		}
		else if(not no_more_opts && sv.starts_with("--"))
		{
			if(sv == "--watch")
			{
				is_watching = true;
			}
			else
			{
				zpr::fprintln(stderr, "unsupported option '{}'", sv);
				return 1;
			}
		}
	}

	if(last != argc)
	{
		zpr::fprintln(stderr, "expected exactly one input file");
		return 1;
	}

	auto input_file = argv[last - 1];
	if(is_watching)
	{
		sap::watch::addFileToWatchList(input_file);

		(void) sap::compile(input_file);

		sap::watch::start(input_file);
	}
	else
	{
		return sap::compile(input_file) ? 0 : 1;
	}
}


extern "C" const char* __asan_default_options()
{
	// The way we do together-compilation (i.e. un-separated compilation)
	// means that this will trigger for a bunch of vtables. no idea why though.
	return "detect_odr_violation=0";
}
