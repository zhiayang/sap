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
	auto font =
		pdf::Font::fromFontFile(&layout_doc.pdfDocument(), font::FontFile::parseFromFile("fonts/SourceSerif4-Regular.otf"));

	auto style = sap::Style {};
	style.set_font(font).set_font_size(pdf::Scalar(12).into(sap::Scalar {}));

	auto default_style = sap::Style()
	                         .set_font(pdf::Font::fromBuiltin(&layout_doc.pdfDocument(), "Times-Roman"))
	                         .set_font_size(pdf::Scalar(12.0).into(sap::Scalar {}))
	                         .set_line_spacing(sap::Scalar(1.0))
	                         .set_pre_paragraph_spacing(sap::Scalar(1.0))
	                         .set_post_paragraph_spacing(sap::Scalar(1.0));

	sap::setDefaultStyle(std::move(default_style));

	layout_doc.setStyle(&style);
	layout_doc.layout(&interpreter);

	auto writer = util::make<pdf::Writer>("test.pdf");
	auto& pdf_doc = layout_doc.render(&interpreter);

	pdf_doc.write(writer);
	writer->close();
}
