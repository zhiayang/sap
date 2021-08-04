// main.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "pdf/pdf.h"
#include "pdf/font.h"
#include "pdf/text.h"

#include "otf/otf.h"

#include <zpr.h>

int main(int argc, char** argv)
{
	auto writer = util::make<pdf::Writer>("test.pdf");
	auto doc = util::make<pdf::Document>();

	auto font = pdf::Font::fromBuiltin("Times-Roman");
	auto f2 = pdf::Font::fromFontFile(doc, otf::OTFont::parseFromFile("Meiryo.ttf"));

	auto p1 = util::make<pdf::Page>();
	auto p2 = util::make<pdf::Page>();

	auto txt = util::make<pdf::Text>();
	txt->setFont(font, pdf::mm(20));
	txt->moveAbs(pdf::mm(10, 200));
	txt->addText("hello, world");

	p1->addObject(txt);

	auto txt2 = util::make<pdf::Text>();
	txt2->setFont(f2, pdf::mm(17));
	txt2->moveAbs(pdf::mm(10, 150));
	txt2->addText("\x09\xb9");

	p1->addObject(txt2);

	doc->addPage(p1);
	doc->addPage(p2);
	doc->write(writer);

	writer->close();


	// otf::OTFont::parseFromFile("SourceSansPro-Regular.ttf");
	// otf::OTFont::parseFromFile("Meiryo.ttf");
	// otf::OTFont::parseFromFile("Meiryo-Italic.ttf");
	// otf::OTFont::parseFromFile("SF-Pro.ttf");
	// otf::OTFont::parseFromFile("DejaVuSansMono.ttf");
	// otf::OTFont::parseFromFile("MyriadPro-Regular.ttf");
	// otf::OTFont::parseFromFile("subset.ttf");
}
