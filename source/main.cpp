// main.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cstdlib>

#include <zpr.h>

#include "sap.h"

#include "pdf/pdf.h"
#include "pdf/font.h"
#include "pdf/text.h"

#include "font/font.h"

constexpr const char* para1 =
	"何 the fuck did you just fucking 言います about 私, you 小さい bitch ですか? 私’ll\n"
	"have あなた know that 私 graduated top of my class in 日本語 3, and 私’ve been involved\n"
	"in 色々な 日本語 tutoring sessions, and 私 have over ３００ perfect test scores. 私\n"
	"am trained in 漢字, and 私 is the top letter writer in all of southern California. あなた are\n"
	"nothing to 私 but just another weaboo. 私 will 殺す anata the fuck out with vocabulary\n"
	"the likes of which has never been 見ます’d before on this continent, mark 私の fucking words.\n"
	"あなた thinks あなた can get away with 話しますing that くそ to 私 over the インターネット? 思う again,\n"
	"fucker. As we 話します, 私 am contacting 私の secret ネット of オタクs across the USA,\n"
	"and あなたの IP is being traced right now so you better 準備します for the ame, ウジ虫. The 雨\n"
	"that 殺す’s the pathetic 小さい thing あなた calls あなたの life. You’re fucking 死にました’d, 赤ちゃん.\n"
	;
	// "\n"

constexpr const char* para2 =
	"My name is Yoshikage Kira. I'm 33 years old. My house is in the northeast section of Morioh,\n"
	"where all the villas are, and I am not married. I work as an employee for the Kame Yu department\n"
	"stores, and I get home every day by 8 PM at the latest. I don't smoke, but I occasionally drink.\n"
	"I'm in bed by 11 PM, and make sure I get eight hours of sleep, no matter what. After having a\n"
	"glass of warm milk and doing about twenty minutes of stretches before going to bed, I usually\n"
	"have no problems sleeping until morning. Just like a baby, I wake up without any fatigue or stress\n"
	"in the morning. I was told there were no issues at my last check-up. I'm trying to explain that\n"
	"I'm a person who wishes to live a very quiet life. I take care not to trouble myself with any enemies,\n"
	"like winning and losing, that would cause me to lose sleep at night. That is how I deal with society,\n"
	"and I know that is what brings me happiness. Although, if I were to fight I wouldn't lose to anyone.";


int main(int argc, char** argv)
{
	auto document = sap::Document();
	auto font = pdf::Font::fromFontFile(
		&document.pdfDocument(),
		font::FontFile::parseFromFile("fonts/XCharter-Roman.otf")
	);

	auto para = sap::Paragraph();
	{
		zst::str_view sv = para2;
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
		.set_font_size(pdf::mm(4.8))
		.set_line_spacing(pdf::mm(1))
		.set_layout_box(pdf::cm(10, 10));

	document.setStyle(&style);

	auto page = util::make<sap::Page>();
	page->add(std::move(para));

	document.add(std::move(*page));

	auto writer = util::make<pdf::Writer>("test.pdf");
	auto pdf_doc = document.finalise();

	pdf_doc.write(writer);
	writer->close();




#if 0

	auto writer = util::make<pdf::Writer>("test.pdf");
	auto doc = util::make<pdf::Document>();

	auto font = pdf::Font::fromBuiltin(doc, "Times-Roman");
	auto f2 = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile("fonts/Meiryo.ttf"));
	auto f3 = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile("fonts/XCharter-Roman.otf"));
	auto f4 = pdf::Font::fromFontFile(doc, font::FontFile::parseFromFile("fonts/MyriadPro-Regular.ttf"));

	auto p1 = util::make<pdf::Page>();
	// auto p2 = util::make<pdf::Page>();

	// auto txt1 = util::make<pdf::Text>();
	// txt1->setFont(font, pdf::mm(20));
	// txt1->moveAbs(pdf::mm(10, 220));
	// txt1->addText("hello, world");

	// auto txt2 = util::make<pdf::Text>();
	// txt2->setFont(f2, pdf::mm(17));
	// txt2->moveAbs(pdf::mm(10, 195));
	// txt2->addText("こんにちは、世界");

	// auto txt3 = util::make<pdf::Text>();
	// txt3->setFont(f3, pdf::mm(17));
	// txt3->moveAbs(pdf::mm(10, 150));
	// txt3->addText("AYAYA, world x\u030c");
	// txt3->offset(pdf::mm(0, -20));
	// txt3->addText("ffi AE ff ffl etc");


	// auto txt3 = util::make<pdf::Text>();
	// txt3->setFont(f3, pdf::mm(4.8));
	// txt3->moveAbs(pdf::mm(10, 150));
	// txt3->addText("My");
	// txt3->offset(pdf::mm(6.6, 0));
	// txt3->addText("name");


	// auto txt4 = util::make<pdf::Text>();
	// txt4->setFont(f4, pdf::mm(17));
	// txt4->moveAbs(pdf::mm(10, 90));
	// txt4->addText("AYAYA \x79\xcc\x8c");
	// txt4->offset(pdf::mm(0, -20));
	// txt4->addText("ffi AE ff ffl etc");

	// p1->addObject(txt1);
	// p1->addObject(txt2);
	p1->addObject(txt3);
	// p1->addObject(txt4);

	doc->addPage(p1);
	// doc->addPage(p2);
	doc->write(writer);

	writer->close();


	font::FontFile::parseFromFile("fonts/Meiryo.ttf");
	font::FontFile::parseFromFile("fonts/subset_Meiryo.ttf");
	font::FontFile::parseFromFile("fonts/meiryo-subset.ttf");
	font::FontFile::parseFromFile("tmp/meiryo-subset.ttf");
	// font::FontFile::parseFromFile("fonts/meiryo-subset.ttf");
	// font::FontFile::parseFromFile("fonts/meiryo-subset.ttf");

	// font::FontFile::parseFromFile("fonts/myriad-subset.ttf");
	// font::FontFile::parseFromFile("fonts/MyriadPro-Regular.ttf");


	// font::FontFile::parseFromFile("fonts/subset_Meiryo.ttf");

	// font::FontFile::parseFromFile("fonts/SourceSansPro-Regular.ttf");
	// font::FontFile::parseFromFile("fonts/Meiryo.ttf");
	// font::FontFile::parseFromFile("fonts/Meiryo-Italic.ttf");
	// font::FontFile::parseFromFile("fonts/SF-Pro.ttf");
	// font::FontFile::parseFromFile("fonts/DejaVuSansMono.ttf");
	// font::FontFile::parseFromFile("fonts/dejavu-subset.ttf");
	// font::FontFile::parseFromFile("fonts/MyriadPro-Regular.ttf");
	// font::FontFile::parseFromFile("fonts/myriad-subset.ttf");
	// font::FontFile::parseFromFile("fonts/XCharter-Roman.otf");
	// font::FontFile::parseFromFile("fonts/xcharter-subset.otf");
#endif
}
