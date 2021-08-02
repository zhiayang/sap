// main.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "pdf/pdf.h"

#include <zpr.h>

int main(int argc, char** argv)
{
	zpr::println("Hello, world!");

	auto writer = util::make<pdf::Writer>("test.pdf");


	auto doc = util::make<pdf::Document>();
	doc->write(writer);


	writer->close();
}
