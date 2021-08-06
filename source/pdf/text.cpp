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

		uint32_t next_gid = 0;

		while(sv.size() > 0 || next_gid != 0)
		{
			uint32_t gid = next_gid;
			next_gid = 0;

			if(gid == 0)
			{
				auto cp = unicode::consumeCodepointFromUtf8(sv);
				gid = font->getGlyphIdFromCodepoint(cp);
			}

			// we consumed one already!
			if(sv.size() > 0)
			{
				auto next_cp = unicode::consumeCodepointFromUtf8(sv);
				next_gid = font->getGlyphIdFromCodepoint(next_cp);
			}


			// TODO: use CMap or something to handle this situation.
			if(gid > 0xffff || next_gid > 0xffff)
				pdf::error("TODO: sorry");

			// TODO: this is pretty suboptimal output
			auto append_glyph = [&ret, &font](uint32_t g) {
				if(font->encoding_kind == Font::ENCODING_CID)
					ret += zpr::sprint("<{02x}{02x}>", (g & 0xff00) >> 8, g & 0xff);
				else
					ret += zpr::sprint("<{02x}>", g & 0xff);
			};

			auto kerning_pair = font->getKerningForGlyphs(gid, next_gid);
			if(kerning_pair.has_value())
			{
				/*
					TODO: honestly, i don't know how this is "supposed" to be handled. Our values
					are the same as what lualatex puts out for the font, so that's ok. the problem
					is that, according to the spec, adj1 is supposed to apply to the first glyph and
					adj2 to the second one (duh).

					except that all the kerning pairs only have adjustments for the first in the pair,
					ie. their effect would be to move the first glyph closer to the second one, instead
					of (logically) the second one closer to the first.

					again, how lualatex does this is (apparently) to use adj1 to move the second glyph
					instead (which produces the same graphical effect).

					so far i haven't found a font that has a non-zero adj2 (and there's a warning here in
					case such a font appears), so for now we just copy what lualatex (allegedly) does.
				*/
				auto adj1 = -1 * kerning_pair->first.horz_advance;
				auto adj2 = -1 * kerning_pair->second.horz_advance;

				if(adj2 != 0)
					zpr::println("warning: unsupported case where adj2 is not 0");

				append_glyph(gid);

				if(adj1 != 0)
					ret += zpr::sprint("{}", adj1);
			}
			else
			{
				append_glyph(gid);
			}
		}

		return ret;
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
			this->commands += zpr::sprint(" [{}] TJ", encode_text_with_font(this->font, text));
	}

	void Text::addText(Scalar offset, zst::str_view text)
	{
		if(!this->in_group)
			pdf::error("addText() with offset can only be used after beginGroup()");

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
