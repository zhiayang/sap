// main.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "pdf/pdf.h"

#include <zpr.h>

int main(int argc, char** argv)
{
	auto writer = util::make<pdf::Writer>("test.pdf");
	auto doc = util::make<pdf::Document>();

	auto p1 = util::make<pdf::Page>();
	auto p2 = util::make<pdf::Page>();
	auto p3 = util::make<pdf::Page>();
	auto p4 = util::make<pdf::Page>();






	doc->addPage(p1);
	doc->addPage(p2);
	doc->addPage(p3);
	doc->addPage(p4);
	doc->write(writer);

	writer->close();
}
