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
		Text() : text("") { }
		Text(std::string text) : text(std::move(text)) { }

		virtual std::string serialise(const Page* page) const override;

		Coord position { };
		std::string text { };

		Font* font { };
		Scalar font_height { };
	};
}
