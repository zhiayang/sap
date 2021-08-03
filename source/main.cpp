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

	auto p1 = util::make<pdf::Page>();
	auto p2 = util::make<pdf::Page>();

	auto txt = util::make<pdf::Text>();
	txt->setFont(font, pdf::mm(20));
	txt->moveAbs(pdf::mm(10, 200));
	txt->addText("hello, world");

	p1->addObject(txt);

	doc->addPage(p1);
	doc->addPage(p2);
	doc->write(writer);

	writer->close();


	// otf::OTFont::parseFromFile("SourceSansPro-Regular.ttf");
	// otf::OTFont::parseFromFile("Meiryo.ttf");
	// otf::OTFont::parseFromFile("Meiryo-Italic.ttf");
	// otf::OTFont::parseFromFile("SF-Pro.ttf");
	// otf::OTFont::parseFromFile("DejaVuSansMono.ttf");
	// otf::OTFont::parseFromFile("subset.ttf");
}
