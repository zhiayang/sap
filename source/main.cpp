// main.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cstdlib>

#include <zpr.h>

#include "sap.h"
#include "util.h"

#include "pdf/font.h"
#include "pdf/pdf.h"
#include "pdf/text.h"

#include "font/font.h"

#include "sap/frontend.h"

#include "interp/state.h"
#include "interp/tree.h"


int main(int argc, char** argv)
{
	if(argc != 2)
	{
		zpr::println("Usage: {} <input file>", argv[0]);
		return 1;
	}

	auto filename = std::string(argv[1]);
	auto [buf, size] = util::readEntireFile(filename);

	auto interpreter = sap::interp::Interpreter();

	auto document = sap::frontend::parse(filename, { (char*) buf, size });

	auto layout_doc = sap::layout::Document();
	/* auto layout_doc = sap::layout::createDocumentLayout(&interpreter, document); */

	auto font_set = [&]() {
		// clang-format off
		// fuck you idiots at llvm who have never written a single line of code in your entire sad lives
		auto regular_path = font::findFontPath(         //
			{ "Source Serif", "Noto Serif", "Serif" },  //
			"Regular",                                  //
			/* { "TrueType", "CFF" }                       // */
			{ "CFF" }  //
		).value_or("fonts/SourceSerif4-Regular.otf");
		auto regular = layout_doc.addFont(font::FontFile::parseFromFile(regular_path));

		auto italic_path = font::findFontPath(          //
			{ "Source Serif", "Noto Serif", "Serif" },  //
			"Italic",                                   //
			/* { "TrueType", "CFF" }                       // */
			{ "CFF" }  //
		).value_or("fonts/SourceSerif4-It.otf");
		auto italic = layout_doc.addFont(font::FontFile::parseFromFile(italic_path));

		auto bold_path = font::findFontPath(            //
			{ "Source Serif", "Noto Serif", "Serif" },  //
			"Bold",                                     //
			/* { "TrueType", "CFF" }                       // */
			{ "CFF" }
		).value_or("fonts/SourceSerif4-Bold.otf");
		auto bold = layout_doc.addFont(font::FontFile::parseFromFile(bold_path));

		auto boldit_path = font::findFontPath(          //
			{ "Source Serif", "Noto Serif", "Serif" },  //
			"Bold Italic",                              //
			/* { "TrueType", "CFF" }                       // */
			{ "CFF" }  //
		).value_or("fonts/SourceSerif4-BoldIt.otf");
		auto boldit = layout_doc.addFont(font::FontFile::parseFromFile(boldit_path));

		return sap::FontSet(regular, italic, bold, boldit);
		// clang-format on
	}();

	auto main_style = sap::Style {};
	main_style //
	    .set_font_set(font_set)
	    .set_font_style(sap::FontStyle::Regular)
	    .set_font_size(pdf::Scalar(12).into(sap::Scalar {}));

	auto actual_style = sap::Style::combine(&main_style, layout_doc.style());

	layout_doc.setStyle(actual_style);

	document.evaluateScripts(&interpreter);
	document.processWordSeparators();
	layout_doc.layout(&interpreter, document);

	auto writer = pdf::Writer(zst::str_view(filename).drop_last(4).str() + ".pdf");
	layout_doc.write(&writer);
	writer.close();
}
