// text.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include "pdf/units.h"
#include "pdf/pageobject.h"

namespace pdf
{
	struct Font;
	struct Page;

	struct Text : PageObject
	{
		virtual std::string serialise(const Page* page) const override;

		void setFont(Font* font);
		void setFont(Font* font, Scalar height);
		void setFontHeight(Scalar height);

		void moveAbs(Coord pos);
		void offset(Coord offset);
		void addText(zst::str_view text);

		void startGroup();
		void addText(Scalar offset, zst::str_view text);
		void endGroup();

	private:
		std::string commands;
		Scalar font_height;
		Font* font;

		bool in_group = false;
	};
}
