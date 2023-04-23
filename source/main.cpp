// main.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#define ZARG_IMPLEMENTATION
#include <zarg.h>

#include "pdf/font.h"
#include "pdf/writer.h"

#include "sap/paths.h"
#include "sap/config.h"
#include "sap/frontend.h"

#include "tree/document.h"
#include "tree/container.h"

#include "layout/document.h"
#include "interp/interp.h"

#if !defined(SAP_PREFIX)
#error SAP_PREFIX must be defined!
#endif

#if 0
TODO:
scripting:
- dereferencing optionals might need to clone them (which is weird)

layout:
- dijkstra linebreaking might accidentally make an extra page;
	- when rendering, use some mechanism (eg. proxy object) to only make the page if
		someone tried to actually put stuff in it (or access it)
- control comma spacing stretch as well (similar to sentence ending)

#endif

namespace sap
{
	bool compile(zst::str_view input_file, zst::str_view output_file)
	{
		auto interp = interp::Interpreter();
		auto file = interp.loadFile(input_file);

		// load microtype configs
		for(auto& lib_path : paths::librarySearchPaths())
		{
			auto tmp = stdfs::path(lib_path) / "data" / "microtype";
			if(not stdfs::exists(tmp))
				continue;

			for(auto& de : stdfs::directory_iterator(tmp))
			{
				if(not de.is_regular_file() || de.path().extension() != ".cfg")
					continue;

				util::log("loading microtype cfg '{}'", de.path().filename().string());

				auto cfg = interp.loadFile(de.path().string()).chars();
				while(not cfg.empty())
				{
					auto ret = config::parseMicrotypeConfig(cfg);
					if(ret.is_err())
					{
						ErrorMessage(Location::builtin(),
						    zpr::sprint("failed to load {}: {}", de.path().filename().string(), ret.error()))
						    .display();
						return false;
					}

					interp.addMicrotypeConfig(std::move(*ret));
				}
			}
		}


		auto document = frontend::parse(input_file, file.chars());
		if(document.is_err())
			return document.error().display(), false;

		auto layout_doc = document.unwrap().layout(&interp);
		if(layout_doc.is_err())
			return layout_doc.error().display(), false;

		interp.setCurrentPhase(ProcessingPhase::Render);

		auto writer = pdf::Writer(output_file);
		layout_doc.unwrap()->write(&writer);
		writer.close();

		return true;
	}

	static bool g_draft_mode = false;
	bool isDraftMode()
	{
		return g_draft_mode;
	}
}



int main(int argc, char** argv)
{
	auto args =
	    zarg::Parser()
	        .add_option('o', true, "output filename")
	        .add_option('I', true, "additional include search path")
	        .add_option('L', true, "additional library search path")
	        .add_option("watch", false, "watch mode: automatically recompile when files change")
	        .add_option("draft", false, "draft mode")
	        .allow_options_after_positionals(true)
	        .parse(argc, argv)
	        .set();

	bool is_watching = args.options.contains("watch");
	sap::g_draft_mode = args.options.contains("draft");

	if(args.positional.size() != 1)
	{
		zpr::fprintln(stderr, "expected exactly one input file");
		exit(1);
	}

	// add the default search paths
	sap::paths::addLibrarySearchPath(stdfs::path(SAP_PREFIX) / "lib" / "sap");
	sap::paths::addIncludeSearchPath(stdfs::path(SAP_PREFIX) / "include" / "sap");

	auto filename = args.positional[0];
	auto abs_filename = stdfs::weakly_canonical(filename);

	auto input_file = abs_filename.string();

	auto output_name = args.options["o"].value.value_or(stdfs::path(abs_filename).replace_extension(".pdf"));
	auto output_file = stdfs::weakly_canonical(output_name).string();

	// change directory to the input file so that all searches are (by default) relative to it,
	// regardless of our actual CWD
	stdfs::current_path(abs_filename.parent_path());

	if(is_watching)
	{
		(void) sap::compile(input_file, output_file);

		sap::watch::addFileToWatchList(input_file);
		sap::watch::start(input_file, output_file);
	}
	else
	{
		return sap::compile(input_file, output_file) ? 0 : 1;
	}
}


extern "C" const char* __asan_default_options()
{
	// The way we do together-compilation (i.e. un-separated compilation)
	// means that this will trigger for a bunch of vtables. no idea why though.
	return "detect_odr_violation=0";
}
