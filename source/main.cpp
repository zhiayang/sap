// main.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#define ZARG_IMPLEMENTATION
#include <zarg.h>

#include "sap/paths.h"

#if !defined(SAP_PREFIX)
#error SAP_PREFIX must be defined!
#endif

#if 0
TODO:
scripting
---------
[ ] decide whether we want positional struct field arguments or not
	- i think the { x } --> { x: x } shorthand was added; this kinda conflicts
		with positional fields

[ ] heckin generics plz

[ ] GO BACK TO UNIQUE_PTR PLS


layout
------
[ ] 'em' doesn't pass down properly in InlineSpans, apparently
	fn foo() -> Inline { make_text("hi").apply_style({ font_size: 2em }); }
	foo().apply_style({ font_size: 20pt });

	this is most likely because we override the style, rather than doing the "correct"
	thing of doing a relative computation when the unit is relative.

[ ] dijkstra linebreaking might accidentally make an extra page;
	- when rendering, use some mechanism (eg. proxy object) to only make the page if
		someone tried to actually put stuff in it (or access it)

[ ] control comma spacing stretch as well (similar to sentence ending)


misc
----
[ ] XML metadata and/or declare PDF/A conformance

[ ] url annotations

[ ] actually use the `-I` and `-L` arguments

#endif


namespace sap
{
	extern void set_draft_mode(bool);
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

	if(args.positional.size() != 1)
	{
		zpr::fprintln(stderr, "expected exactly one input file");
		return 1;
	}

	sap::set_draft_mode(args.options.contains("draft"));
	bool is_watching = args.options.contains("watch");
	if(is_watching && not sap::watch::isSupportedPlatform())
	{
		zpr::fprintln(stderr, "--watch mode is not supported on this platform");
		return 1;
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

	// we'll fix leaks later.
	return "detect_odr_violation=0:detect_leaks=0";
}
