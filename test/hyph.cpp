// hyph.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "misc/hyphenator.h"

int main(int argc, char** argv)
{
	std::string hyph_path = "lib/sap/data/hyphenation/hyph-en-gb.tex";
	if(argc == 3)
		hyph_path = argv[2];

	sap::hyph::Hyphenator hyph = sap::hyph::Hyphenator::parseFromFile(hyph_path);

	auto test = [&](zst::wstr_view w) {
		auto points = hyph.computeHyphenationPoints(w);
		assert(points.size() == w.size() + 1);
		for(size_t i = 0; i < w.size(); ++i)
		{
			if(points[i] > 0)
				zpr::print("{}{}", (int) points[i], (char) w[i]);
			else
				zpr::print("{}", (char) w[i]);
		}

		if(points.back() > 0)
			zpr::println("{}", (int) points.back());
		else
			zpr::println("");

		for(size_t i = 0; i < w.size(); ++i)
		{
			if(points[i] % 2 == 1)
				zpr::print("-{}", (char) w[i]);
			else
				zpr::print("{}", (char) w[i]);
		}

		zpr::println("{}", points.back() % 2 == 1 ? "-" : "");

		for(size_t i = 0; i < w.size(); ++i)
		{
			if(points[i] % 2 == 1 && points[i] > 2)
				zpr::print("-{}", (char) w[i]);
			else
				zpr::print("{}", (char) w[i]);
		}

		zpr::println("{}", points.back() % 2 == 1 ? "-" : "");

		for(size_t i = 0; i < w.size(); ++i)
		{
			if(points[i] % 2 == 1 && points[i] > 4)
				zpr::print("-{}", (char) w[i]);
			else
				zpr::print("{}", (char) w[i]);
		}

		zpr::println("{}", points.back() % 2 == 1 ? "-" : "");
		zpr::println("");
	};

	if(argc >= 2)
	{
		auto s = unicode::u32StringFromUtf8(zst::str_view((const char*) argv[1]).bytes());
		test(s);
	}
	else
	{
		test(U"hotdog");
		test(U"laptop");
		test(U"computer");
		test(U"macbookpro");
		test(U"hyphenation");
		test(U"respectively");
		test(U"irregardless");
		test(U"creativity");
		test(U"university");
		test(U"universities");
	}
}

bool sap::compile(zst::str_view input_file, zst::str_view output_file)
{
	return false;
}

bool sap::isDraftMode()
{
	return false;
}
