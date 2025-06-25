// main.cpp
// Copyright (c) 2021, yuki
// SPDX-License-Identifier: Apache-2.0

#define ZARG_IMPLEMENTATION
#include <zarg.h>

#include "sap/config.h"

#if !defined(SAP_PREFIX)
#error SAP_PREFIX must be defined!
#endif

/*
BUGS!!!!

[ ] line spacing doesn't seem to carry properly into lists
[ ] list items are not justified (and manually setting alignment does nothing)
[ ] disabling page number inside script block does not work
[ ] vbox items do not properly start on a new page; eg.
        <para> <para> | <header>
        if the | is a pagebreak, then the inter-item spacing in the vbox appears to be
        broken somehow, and <header> starts higher up on the page than it should (vs. if we
        had a manual \page_break() at the '|' point)

        tldr paragraph spacing across pages is possibly fucked


TODO:
scripting
---------
[ ] idk if the limitation of having '//' need struct literals is legit. could imagine
    a scenario where you incrementally build up the replacement struct and wanna update
    something. unfortunately the way StructUpdateOp is structured makes this annoying.

[ ] auto formatting for printing structs in f-strings?

[ ] percentage DynLengths; something like `50%pw` for 50% of page-width; we can have
    %pw/ph for page w/h, %lw/lh for line, %w/h for the size of the parent container
    would need a bunch of changes in DynLength but hopefully not too much?

[ ] DynLengths are generally broken; they are not consistently resolved at the usage
    site, due to the use of ev->currentStyle() and certain structs (eg. PathStyle) storing
    Lengths rather than DynLengths. We don't want to leak Styles into the pdf layer, so
    that shouldn't change, but then we need a way to consistently resolve the DynLengths at
    the correct place.

    eg. `2em` should always be relative to whatever object that it is applied to, regardless
    of the call/instantiation site.

[ ] heckin generics plz

[ ] GO BACK TO UNIQUE_PTR PLS

[ ] would be nice to have a `style` parameter for the builtin make_* functions
    instead of needing apply_style every damn time


layout
------
[ ] we should have a small table of missing glyph substitutions,
    eg. left/right ("smart") quotes to normal '/" quotes, if the font doesn't have it (eg. 14 builtin ones)


[ ] 'em' doesn't pass down properly in InlineSpans, apparently
    fn foo() -> Inline { make_text("hi").apply_style({ font_size: 2em }); }
    foo().apply_style({ font_size: 20pt });

    this is most likely because we override the style, rather than doing the "correct"
    thing of doing a relative computation when the unit is relative. though this would be a pain if
    you actually wanted to override the font size to absolute 20pt, not 2em * 20pt


[ ] borders will probably break if they need to span more than one page
    we can have something to choose whether or not the top/bottom is drawn at the split, but the issue
    is that those lines will take up space --- which is not (and cannot be) accounted for in the LayoutSize...

    latex's mdframed seems to handle this situation, but that's prolly because latex doesn't layout like us. an
    easy hack for now would just be to eat into the margin to draw the in-between-page top/bottom borders.


[ ] dijkstra linebreaking might accidentally make an extra page;
    - when rendering, use some mechanism (eg. proxy object) to only make the page if
        someone tried to actually put stuff in it (or access it)

[ ] control comma spacing stretch as well (similar to sentence ending)


[ ] use masks to generate proper rounded-corner borders. we should be able to specify radii
    for each of the 4 corners, and the capstyle property of border path segments should be ignored.



misc
----
[ ] need a better ErrorMessage struct or something

[ ] XML metadata and/or declare PDF/A conformance

[ ] url annotations

[ ] actually use the `-I` and `-L` arguments

*/

namespace sap
{
	extern void set_draft_mode(bool _);

	static stdfs::path s_invocation_cwd;
	stdfs::path getInvocationCWD()
	{
		return s_invocation_cwd;
	}
}

int main(int argc, char** argv)
{
	auto args = zarg::Parser()
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
	sap::paths::addLibrarySearchPath((stdfs::path(SAP_PREFIX) / "lib" / "sap").string());
	sap::paths::addIncludeSearchPath((stdfs::path(SAP_PREFIX) / "include" / "sap").string());

	auto filename = args.positional[0];
	auto abs_filename = stdfs::weakly_canonical(filename);

	auto input_file = abs_filename.string();

	auto output_name = args.options["o"].value.value_or(stdfs::path(abs_filename).replace_extension(".pdf").string());
	auto output_file = stdfs::weakly_canonical(output_name).string();

	// change directory to the input file so that all searches are (by default)
	// relative to it, regardless of our actual CWD
	sap::s_invocation_cwd = stdfs::current_path();
	stdfs::current_path(abs_filename.parent_path());

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
