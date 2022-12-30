// main.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for readEntireFile

#include "pdf/units.h"  // for Scalar
#include "pdf/writer.h" // for Writer

#include "sap/style.h"    // for Style
#include "sap/units.h"    // for Scalar
#include "sap/fontset.h"  // for FontSet, FontStyle, FontStyle::Regular
#include "sap/document.h" // for Document
#include "sap/frontend.h" // for parse

#include "font/font.h" // for findFontPath, FontFile
#include "font/search.h"

#include "interp/tree.h"   // for Document
#include "interp/interp.h" // for Interpreter

#if 0
CLEANUPS:

- use magic .into() for units
- cleanup the pdf::Font / font::FontFile nonsense
- "fontconfig" for macos
- maybe wrapper around (const Style*)
- maybe s/layout/print/ or something
	- rename layout() methods to print() or something
	- さあぁぁぁぁ

#endif

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

	auto font_family = [&]() {
		using namespace font;

		std::vector<std::string> prefs = { "Source Serif 4", GENERIC_SERIF };

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

		return sap::FontSet(regular, italic, bold, boldit);
	}();

	auto main_style = sap::Style {};
	main_style //
	    .set_fontset(font_family)
	    .set_font_style(sap::FontStyle::Regular)
	    .set_font_size(pdf::PdfScalar(12).into());

	auto actual_style = layout_doc.style()->extend(&main_style);

	layout_doc.setStyle(actual_style);

	document.evaluateScripts(&interpreter);
	document.processWordSeparators();
	layout_doc.layout(&interpreter, document);

	auto out_path = std::filesystem::path(filename).replace_extension(".pdf");
	auto writer = pdf::Writer(out_path.string());
	layout_doc.write(&writer);
	writer.close();
}
