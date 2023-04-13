// main.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#define ZARG_IMPLEMENTATION
#include <zarg.h>

#include "pdf/font.h"
#include "pdf/writer.h"

#include "sap/paths.h"
#include "sap/frontend.h"

#include "tree/document.h"
#include "tree/container.h"

#include "layout/document.h"
#include "interp/interp.h"

#if !defined(SAP_PREFIX)
#error SAP_PREFIX must be defined!
#endif

#if 0
CLEANUP:
- maybe wrapper around (const Style*)

TODO:
scripting:
- dereferencing optionals might need to clone them (which is weird)

layout:
- microtype-like stuff (hanging punctuation)
- dijkstra linebreaking might accidentally make an extra page;
	- when rendering, use some mechanism (eg. proxy object) to only make the page if
		someone tried to actually put stuff in it (or access it)
- control comma spacing stretch as well (similar to sentence ending)

#endif

bool sap::compile(zst::str_view input_file, zst::str_view output_file)
{
	auto interp = interp::Interpreter();
	auto file = interp.loadFile(input_file);

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

int main(int argc, char** argv)
{
	auto pppp = zarg::Parser();
	pppp.add_option('o', true, "output filename")
	    .add_option('I', true, "additional include search path")
	    .add_option('L', true, "additional library search path")
	    .add_option("watch", false, "watch mode: automatically recompile when files change")
	    .allow_options_after_positionals(true) //
	    ;

	auto args = pppp.parse(argc, argv).set();
	bool is_watching = args.options.contains("watch");

	if(args.positional.size() != 1)
	{
		zpr::fprintln(stderr, "expected exactly one input file");
		exit(1);
	}

	auto filename = args.positional[0];
	auto abs_filename = stdfs::weakly_canonical(filename);

	// change directory to the input file so that all searches are (by default) relative to it,
	// regardless of our actual CWD
	stdfs::current_path(abs_filename.parent_path());

	auto input_file = abs_filename.string();
	auto output_file = args.options["o"].value.value_or(stdfs::path(abs_filename).replace_extension(".pdf"));

	// add the default search paths
	sap::paths::addLibrarySearchPath(stdfs::path(SAP_PREFIX) / "lib" / "sap");
	sap::paths::addIncludeSearchPath(stdfs::path(SAP_PREFIX) / "include" / "sap");

	if(is_watching)
	{
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
