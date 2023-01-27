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
- x= should be synthesised from x and = if possible (for x in [+, -, *, /, %])
	- there's a **LOT** of code dupe between binop.cpp and assop.cpp
- unify script handling between ScriptCall and Paragraph::evaluate_scripts

layout:
- em/en dashes
- smart quotes
- spaces after the end of a sentence should be slightly longer than spaces between words
- page numbers
- microtype-like stuff (hanging punctuation)
- dijkstra linebreaking might accidentally make an extra page;
	- when rendering, use some mechanism (eg. proxy object) to only make the page if
		someone tried to actually put stuff in it (or access it)

#endif

namespace sap
{
	void compile(zst::str_view filename);
}

void sap::compile(zst::str_view filename)
{
	auto interp = interp::Interpreter();
	auto file = interp.loadFile(filename);

	auto document = frontend::parse(filename, file.chars());
	auto layout_doc = document.layout(&interp);



	interp.setCurrentPhase(ProcessingPhase::Render);
	auto out_path = std::filesystem::path(filename.str()).replace_extension(".pdf");
	auto writer = pdf::Writer(out_path.string());
	layout_doc->write(&writer);
	writer.close();
}

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		zpr::println("Usage: {} <input file>", argv[0]);
		return 1;
	}

	sap::compile(argv[1]);
}


extern "C" const char* __asan_default_options()
{
	// The way we do together-compilation (i.e. un-separated compilation)
	// means that this will trigger for a bunch of vtables. no idea why though.
	return "detect_odr_violation=0";
}
