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

namespace sap
{
	pdf::Document compile()
	{
		auto document = sap::layout::Document();

		auto cs = interp::Interpreter();
		return std::move(document.render(&cs));
	}
}

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		zpr::println("Usage: {} <input file>", argv[0]);
		return 1;
	}
	std::string filename(argv[1]);
	auto [buf, size] = util::readEntireFile(filename);
	auto document = sap::frontend::parse(filename, { (char*) buf, size });

	auto interpreter = sap::interp::Interpreter();

	auto layout_doc = sap::layout::createDocumentLayout(&interpreter, document);

	auto font_set = [&]() {
		auto doc = &layout_doc.pdfDocument();

		// clang-format off
		// fuck you idiots at llvm who have never written a single line of code in your entire sad lives
		auto regular_path = font::findFontPath(         //
			{ "Source Serif", "Noto Serif", "Serif" },  //
			"Regular",                                  //
			/* { "TrueType", "CFF" }                       // */
			{ "CFF" }  //
		).value_or("fonts/SourceSerif4-Regular.otf");

		auto regular = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile(regular_path));

		auto italic_path = font::findFontPath(          //
			{ "Source Serif", "Noto Serif", "Serif" },  //
			"Italic",                                   //
			/* { "TrueType", "CFF" }                       // */
			{ "CFF" }  //
		).value_or("fonts/SourceSerif4-It.otf");

		auto italic = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile(italic_path));

		auto bold_path = font::findFontPath(            //
			{ "Source Serif", "Noto Serif", "Serif" },  //
			"Bold",                                     //
			/* { "TrueType", "CFF" }                       // */
			{ "CFF" }
		).value_or("fonts/SourceSerif4-Bold.otf");

		auto bold = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile(bold_path));

		auto boldit_path = font::findFontPath(          //
			{ "Source Serif", "Noto Serif", "Serif" },  //
			"Bold Italic",                              //
			/* { "TrueType", "CFF" }                       // */
			{ "CFF" }  //
		).value_or("fonts/SourceSerif4-BoldIt.otf");

		auto boldit = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile(boldit_path));

		return sap::FontSet(regular, italic, bold, boldit);
		// clang-format on
	}();

	auto default_font_set = sap::FontSet(  //
		pdf::Font::fromBuiltin(&layout_doc.pdfDocument(), "Times-Roman"),
		pdf::Font::fromBuiltin(&layout_doc.pdfDocument(), "Times-Italic"),
		pdf::Font::fromBuiltin(&layout_doc.pdfDocument(), "Times-Bold"),
		pdf::Font::fromBuiltin(&layout_doc.pdfDocument(), "Times-BoldItalic"));

	auto main_style = sap::Style {};
	main_style  //
		.set_font_set(font_set)
		.set_font_style(sap::FontStyle::Regular)
		.set_font_size(pdf::Scalar(12).into(sap::Scalar {}));

	auto default_style =
		sap::Style()
			.set_font_set(default_font_set)
			.set_font_style(sap::FontStyle::Regular)
			.set_font_size(pdf::Scalar(12.0).into(sap::Scalar {}))
			.set_line_spacing(sap::Scalar(1.0))
			.set_pre_paragraph_spacing(sap::Scalar(1.0))
			.set_post_paragraph_spacing(sap::Scalar(1.0));

	sap::setDefaultStyle(std::move(default_style));

	layout_doc.setStyle(&main_style);
	layout_doc.layout(&interpreter);

	auto writer = util::make<pdf::Writer>("test.pdf");
	auto& pdf_doc = layout_doc.render(&interpreter);

	pdf_doc.write(writer);
	writer->close();
}
