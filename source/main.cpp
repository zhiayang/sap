// main.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "pdf/pdf.h"
#include "pdf/font.h"
#include "pdf/text.h"

#include "font/font.h"

#include <zpr.h>

int main(int argc, char** argv)
{
#if 1
	auto writer = util::make<pdf::Writer>("test.pdf");
	auto doc = util::make<pdf::Document>();

	auto font = pdf::Font::fromBuiltin("Times-Roman");
	auto f2 = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile("Meiryo.ttf"));
	auto f3 = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile("XCharter-Roman.otf"));
	auto f4 = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile("MyriadPro-Regular.ttf"));

	auto p1 = util::make<pdf::Page>();
	// auto p2 = util::make<pdf::Page>();

	auto txt1 = util::make<pdf::Text>();
	txt1->setFont(font, pdf::mm(20));
	txt1->moveAbs(pdf::mm(10, 220));
	txt1->addText("hello, world");

	auto txt2 = util::make<pdf::Text>();
	txt2->setFont(f2, pdf::mm(17));
	txt2->moveAbs(pdf::mm(10, 195));
	txt2->addText("こんにちは、世界");

	auto txt3 = util::make<pdf::Text>();
	txt3->setFont(f3, pdf::mm(17));
	txt3->moveAbs(pdf::mm(10, 150));
	txt3->addText("AYAYA, world x\u030c");
	txt3->offset(pdf::mm(0, -20));
	txt3->addText("ffi AE ff ffl etc");

	auto txt4 = util::make<pdf::Text>();
	txt4->setFont(f4, pdf::mm(17));
	txt4->moveAbs(pdf::mm(10, 90));
	txt4->addText("AYAYA \x79\xcc\x8c");
	txt4->offset(pdf::mm(0, -20));
	txt4->addText("ffi AE ff ffl etc");

	p1->addObject(txt1);
	p1->addObject(txt2);
	p1->addObject(txt3);
	p1->addObject(txt4);

	doc->addPage(p1);
	// doc->addPage(p2);
	doc->write(writer);

	writer->close();
#endif

	// font::FontFile::parseFromFile("SourceSansPro-Regular.ttf");
	// font::FontFile::parseFromFile("Meiryo.ttf");
	// font::FontFile::parseFromFile("Meiryo-Italic.ttf");
	// font::FontFile::parseFromFile("SF-Pro.ttf");
	// font::FontFile::parseFromFile("DejaVuSansMono.ttf");
	// font::FontFile::parseFromFile("dejavu-subset.ttf");
	// font::FontFile::parseFromFile("MyriadPro-Regular.ttf");
	// font::FontFile::parseFromFile("myriad-subset.ttf");
}
