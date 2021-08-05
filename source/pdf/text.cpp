// text.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <cassert>

#include "util.h"
#include "pdf/font.h"
#include "pdf/text.h"
#include "pdf/page.h"
#include "pdf/misc.h"

namespace pdf
{
	// also encode it.
	static std::string encode_text_with_font(const Font* font, zst::str_view text)
	{
		std::string ret;
		auto sv = text.bytes();
		while(sv.size() > 0)
		{
			auto cp = unicode::codepointFromUtf8(sv);
			auto gid = font->getGlyphIdFromCodepoint(cp);

			// TODO: use CMap or something to handle this situation.
			if(gid > 0xffff)
				pdf::error("TODO: sorry");

			if(font->encoding_kind == Font::ENCODING_CID)
				ret += zpr::sprint("{02x}{02x}", (gid & 0xff00) >> 8, gid & 0xff);
			else
				ret += zpr::sprint("{02x}", gid & 0xff);
		}

		return "<" + ret + ">";
	}



	std::string Text::serialise(const Page* page) const
	{
		if(this->font == nullptr || this->font_height.zero())
			return "";

		return zpr::sprint("BT\n  /{} {} Tf\n {}\nET\n",
			page->getNameForFont(this->font), this->font_height, commands);
	}

	void Text::setFont(Font* font, Scalar height)
	{
		assert(font != nullptr);
		this->font = font;
		this->font_height = height;
	}

	// convention is that all appends to the command list should start with a " ".
	void Text::moveAbs(Coord pos)
	{
		// do this in two steps; first, replace the text matrix with the identity to get to (0, 0),
		// then perform an offset to get to the desired position.
		this->commands += zpr::sprint(" 1 0 0 1 0 0 Tm");
		this->offset(pos);
	}

	void Text::offset(Coord offset)
	{
		this->commands += zpr::sprint(" {} {} Td", offset.x, offset.y);
	}

	void Text::startGroup()
	{
		if(this->in_group)
			pdf::error("text groups (TJ) cannot be nested");

		this->in_group = true;
		this->commands += " [";
	}

	void Text::addText(zst::str_view text)
	{
		if(this->in_group)
			this->commands += zpr::sprint(" {}", encode_text_with_font(this->font, text));
		else
			this->commands += zpr::sprint(" {} Tj", encode_text_with_font(this->font, text));
	}

	void Text::addText(Scalar offset, zst::str_view text)
	{
		this->commands += zpr::sprint(" {} {}", offset, encode_text_with_font(this->font, text));
	}

	void Text::endGroup()
	{
		if(!this->in_group)
			pdf::error("endGroup() can only be used after beginGroup()");

		this->in_group = false;
		this->commands += "]";
	}
}
