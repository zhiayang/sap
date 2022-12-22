// main.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cstdlib>

#include <zpr.h>

#include "sap.h"
#include "util.h"

#include "pdf/pdf.h"
#include "pdf/font.h"
#include "pdf/text.h"

#include "font/font.h"

#include "sap/frontend.h"

#include "interp/tree.h"
#include "interp/state.h"


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
	auto [buf, size] = util::readEntireFile("test.sap");
	auto document = sap::frontend::parse("test.sap", { (char*) buf, size });

	auto interpreter = sap::interp::Interpreter();

	auto layout_doc = sap::layout::createDocumentLayout(&interpreter, document);

	auto font_set = [&]() {
		auto doc = &layout_doc.pdfDocument();
		auto regular = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile("fonts/SourceSerif4-Regular.otf"));
		auto italic = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile("fonts/SourceSerif4-It.otf"));
		auto bold = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile("fonts/SourceSerif4-Bold.otf"));
		auto boldit = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile("fonts/SourceSerif4-BoldIt.otf"));

		return sap::FontSet(regular, italic, bold, boldit);
	}();

	auto default_font_set = sap::FontSet(  //
		pdf::Font::fromBuiltin(&layout_doc.pdfDocument(), "Times-Roman"),
		pdf::Font::fromBuiltin(&layout_doc.pdfDocument(), "Times-Italic"),
		pdf::Font::fromBuiltin(&layout_doc.pdfDocument(), "Times-Bold"),
		pdf::Font::fromBuiltin(&layout_doc.pdfDocument(), "Times-BoldItalic"));

	auto main_style = sap::Style {};
	main_style
		.set_font_set(font_set)  //
		.set_font_style(sap::FontStyle::Regular)
		.set_font_size(pdf::Scalar(12).into(sap::Scalar {}));

	auto default_style = sap::Style()
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
