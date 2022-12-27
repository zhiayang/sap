#include "hyph.h"

int main(int argc, char* argv[])
{
	std::string hyph_path = "hyph-en-gb.pat.txt";
	if(argc == 3)
	{
		hyph_path = argv[2];
	}


	sap::hyph::AsciiHyph hyph = sap::hyph::AsciiHyph::parseFromFile(hyph_path);

	auto test = [&](zst::str_view w) {
		auto points = hyph.computeHyphenationPointsForLowercaseAlphabeticalWord(w);
		assert(points.size() == w.size() + 1);
		for(size_t i = 0; i < w.size(); ++i)
		{
			if(points[i] > 0)
				zpr::print("{}{}", (int) points[i], w[i]);
			else
				zpr::print("{}", w[i]);
		}
		if(points.back() > 0)
			zpr::println("{}", (int) points.back());
		else
			zpr::println("");

		for(size_t i = 0; i < w.size(); ++i)
		{
			if(points[i] % 2 == 1)
				zpr::print("-{}", w[i]);
			else
				zpr::print("{}", w[i]);
		}
		zpr::println("{}", points.back() % 2 == 1 ? "-" : "");

		for(size_t i = 0; i < w.size(); ++i)
		{
			if(points[i] % 2 == 1 && points[i] > 2)
				zpr::print("-{}", w[i]);
			else
				zpr::print("{}", w[i]);
		}
		zpr::println("{}", points.back() % 2 == 1 ? "-" : "");

		for(size_t i = 0; i < w.size(); ++i)
		{
			if(points[i] % 2 == 1 && points[i] > 4)
				zpr::print("-{}", w[i]);
			else
				zpr::print("{}", w[i]);
		}
		zpr::println("{}", points.back() % 2 == 1 ? "-" : "");
		zpr::println("");
	};

	if(argc >= 2)
	{
		test(std::string(argv[1]));
	}
	else
	{
		test("hotdog");
		test("laptop");
		test("computer");
		test("macbookpro");
		test("hyphenation");
		test("respectively");
		test("irregardless");
		test("creativity");
	}
}
