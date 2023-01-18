// main.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for readEntireFile

#include "pdf/font.h"   // for Font
#include "pdf/units.h"  // for PdfScalar
#include "pdf/writer.h" // for Writer

#include "sap/style.h"       // for Style
#include "sap/frontend.h"    // for parse
#include "sap/font_family.h" // for FontSet, FontStyle, FontStyle::Regular

#include "tree/document.h"
#include "layout/document.h" // for Document

#include "interp/interp.h" // for Interpreter

#if 0
CLEANUP:
- maybe wrapper around (const Style*)
- smart quotes
- dijkstra linebreaking might accidentally make an extra page;
	- when rendering, use some mechanism (eg. proxy object) to only make the page if
		someone tried to actually put stuff in it (or access it)

- line height should move the first line, not the second line.

#endif

namespace sap
{
	void compile(zst::str_view filename);
}

void sap::compile(zst::str_view filename)
{
	auto entire_file = util::readEntireFile(filename.str());

	auto interpreter = interp::Interpreter();

	auto document = frontend::parse(filename, zst::str_view((char*) entire_file.get(), entire_file.size()));
	auto layout_doc = layout::Document(&interpreter);

	interpreter.evaluator().pushStyle(layout_doc.style());
	document.layout(&interpreter, &layout_doc);

	auto out_path = std::filesystem::path(filename.str()).replace_extension(".pdf");
	auto writer = pdf::Writer(out_path.string());
	layout_doc.write(&writer);
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
