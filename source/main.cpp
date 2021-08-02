// main.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "pdf/pdf.h"
#include "pdf/font.h"
#include "pdf/text.h"

#include <zpr.h>

int main(int argc, char** argv)
{
	auto writer = util::make<pdf::Writer>("test.pdf");
	auto doc = util::make<pdf::Document>();

	auto font = pdf::Font::fromBuiltin("Times-Roman");

	auto p1 = util::make<pdf::Page>();
	auto p2 = util::make<pdf::Page>();
	// p1->useFont(font);

	auto txt = util::make<pdf::Text>("hello world");
	txt->font_height = pdf::mm(24);
	txt->font = font;
	txt->position = pdf::mm(20, 120);

	p1->addObject(txt);

	doc->addPage(p1);
	doc->addPage(p2);
	doc->write(writer);

	writer->close();
}
