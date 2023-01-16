// main.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for readEntireFile

#include "pdf/font.h"   // for Font
#include "pdf/units.h"  // for PdfScalar
#include "pdf/writer.h" // for Writer

#include "sap/style.h"       // for Style
#include "sap/document.h"    // for Document
#include "sap/frontend.h"    // for parse
#include "sap/font_family.h" // for FontSet, FontStyle, FontStyle::Regular

#include "tree/document.h"

#include "font/tag.h"       // for font
#include "font/handle.h"    // for FontProperties, FontStyle, FontStyle::ITALIC
#include "font/search.h"    // for findFont, GENERIC_SERIF
#include "font/font_file.h" // for FontFile

#include "interp/interp.h" // for Interpreter

#if 0
CLEANUP:

- maybe wrapper around (const Style*)
- smart quotes
- dijkstra linebreaking might accidentally make an extra page;
	- when rendering, use some mechanism (eg. proxy object) to only make the page if
		someone tried to actually put stuff in it (or access it)

#endif

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		zpr::println("Usage: {} <input file>", argv[0]);
		return 1;
	}

	auto filename = std::string(argv[1]);
	auto entire_file = util::readEntireFile(filename);

	auto interpreter = sap::interp::Interpreter();

	auto document = sap::frontend::parse(filename, zst::str_view((char*) entire_file.get(), entire_file.size()));
	auto layout_doc = sap::layout::Document();

	auto font_family = [&]() {
		using namespace font;

		std::vector<std::string> prefs = { "Source Serif 4", "Helvetica", GENERIC_SERIF };

		auto regular_handle = font::findFont(prefs, FontProperties {});
		auto italic_handle = font::findFont(prefs, FontProperties { .style = FontStyle::ITALIC });
		auto bold_handle = font::findFont(prefs, FontProperties { .weight = FontWeight::BOLD });
		auto boldit_handle = font::findFont(prefs, FontProperties { .style = FontStyle::ITALIC, .weight = FontWeight::BOLD });

		assert(regular_handle.has_value());
		assert(italic_handle.has_value());
		assert(bold_handle.has_value());
		assert(boldit_handle.has_value());

		auto regular = layout_doc.addFont(*FontFile::fromHandle(*regular_handle));
		auto italic = layout_doc.addFont(*FontFile::fromHandle(*italic_handle));
		auto bold = layout_doc.addFont(*FontFile::fromHandle(*bold_handle));
		auto boldit = layout_doc.addFont(*FontFile::fromHandle(*boldit_handle));

		return sap::FontFamily(regular, italic, bold, boldit);
	}();

	auto main_style = sap::Style {};
	main_style //
	    .set_font_family(font_family)
	    .set_font_style(sap::FontStyle::Regular)
	    .set_line_spacing(1)
	    .set_paragraph_spacing(sap::Length(5))
	    .set_font_size(pdf::PdfScalar(13).into());

	auto actual_style = layout_doc.style()->extendWith(&main_style);
	layout_doc.setStyle(actual_style);

	interpreter.evaluator().pushStyle(layout_doc.style());

	document.evaluateScripts(&interpreter);
	document.processWordSeparators();
	layout_doc.layout(&interpreter, document);

	auto out_path = std::filesystem::path(filename).replace_extension(".pdf");
	auto writer = pdf::Writer(out_path.string());
	layout_doc.write(&writer);
	writer.close();
}
