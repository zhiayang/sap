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

#include "frontend/lexer.h"

#if 0
constexpr const char* input_text =

	"AVAYAYA V. Vo P. r.";
#else
constexpr const char* input_text = "Bсички хора се раждат свободни и равни по";
	// "My name is Yoshikage Kira. I'm 33 years old. My house is in the northeast section of Morioh,\n"
	// "where all the villas are, and I am not married. I work as an employee for the Kame Yu department\n"
	// "stores, and I get home every day by 8 PM at the latest. I don't smoke, but I occasionally drink.\n"
	// "I'm in bed by 11 PM, and make sure I get eight hours of sleep, no matter what. After having a\n"
	// "glass of warm milk and doing about twenty minutes of stretches before going to bed, I usually\n"
	// "have no problems sleeping until morning. Just like a baby, I wake up without any fatigue or stress\n"
	// "in the morning. I was told there were no issues at my last check-up. I'm trying to explain that\n"
	// "I'm a person who wishes to live a very quiet life. I take care not to trouble myself with any enemies,\n"
	// "like winning and losing, that would cause me to lose sleep at night. That is how I deal with society,\n"
	// "and I know that is what brings me happiness. Although, if I were to fight I wouldn't lose to anyone.\n"
	// "AVAYAYA V. Vo P. r.";
#endif

int main(int argc, char** argv)
{
	auto document = sap::Document();
	auto font = pdf::Font::fromFontFile(
		&document.pdfDocument(),
		// font::FontFile::parseFromFile("fonts/Meiryo.ttf")
		// font::FontFile::parseFromFile("fonts/MyriadPro-Regular.ttf")
		// font::FontFile::parseFromFile("fonts/XCharter-Roman.otf")
		// font::FontFile::parseFromFile("fonts/FDArrayTest257.otf")
		font::FontFile::parseFromFile("fonts/SourceSerif4-Regular.otf")
		// font::FontFile::parseFromFile("fonts/SourceSerif4Variable-Roman.otf")
	);

	auto [ buf, size ] = util::readEntireFile("test.sap");
	auto lexer = sap::frontend::Lexer({ (char*) buf, size }, "test.sap");
	while(true)
	{
		auto tok = lexer.next();
		zpr::println("{} = '{}'", tok.type, tok.text);

		if(tok == sap::frontend::TokenType::EndOfFile)
			break;
	}




#if 1
	auto para = sap::Paragraph();
	{
		zst::str_view sv = input_text;
		while(sv.size() > 0)
		{
			// TODO: unicode whitespace check
			// TODO: windows \r\n handling
			auto i = sv.find_first_of(" \t\r\n");
			if(i == 0)
			{
				sv.remove_prefix(1);
				continue;
			}

			// if there is no more, take the last part and quit.
			if(i == static_cast<size_t>(-1))
			{
				para.add(sap::Word(sap::Word::KIND_LATIN, sv));
				break;
			}
			else
			{
				para.add(sap::Word(sap::Word::KIND_LATIN, sv.take(i)));
				sv.remove_prefix(i + 1);
			}
		}
	}

	auto style = sap::Style {};
	style.set_font(font)
		// .set_font_size(pdf::Scalar(12).into(sap::Scalar{}))
		.set_font_size(dim::mm(4.8))
		.set_line_spacing(dim::mm(3));

	auto default_style = sap::Style()
		.set_font(pdf::Font::fromBuiltin(&document.pdfDocument(), "Times-Roman"))
		.set_font_size(pdf::Scalar(12.0/72.0).into(sap::Scalar{}))
		.set_line_spacing(sap::Scalar(1.0))
		.set_pre_paragraph_spacing(sap::Scalar(1.0))
		.set_post_paragraph_spacing(sap::Scalar(1.0));

	sap::setDefaultStyle(std::move(default_style));

	document.setStyle(&style);
	document.addObject(&para);
	document.layout();

	auto writer = util::make<pdf::Writer>("test.pdf");
	auto pdf_doc = document.render();

	pdf_doc.write(writer);
	writer->close();
#endif
}
