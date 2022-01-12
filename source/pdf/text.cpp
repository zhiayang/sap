// text.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cassert>

#include "util.h"
#include "pdf/font.h"
#include "pdf/text.h"
#include "pdf/page.h"
#include "pdf/misc.h"

namespace pdf
{
	std::string Text::serialise(const Page* page) const
	{
		// note the opening '['
		return zpr::sprint("q BT\n [{} ET Q\n", commands);
	}

	void Text::setFont(const Font* font, Scalar height)
	{
		assert(font != nullptr);
		this->commands += zpr::sprint(" /{} {} Tf", font->getFontResourceName(), height);
		this->used_fonts.push_back(font);
	}

	void Text::insertPDFCommand(zst::str_view sv)
	{
		this->commands += zpr::sprint("] {} [", sv);
	}

	// convention is that all appends to the command list should start with a " ".
	void Text::moveAbs(Vector pos)
	{
		// close the current group, and call the TJ operator.
		this->commands += "] TJ\n";

		// do this in two steps; first, replace the text matrix with the identity to get to (0, 0),
		// then perform an offset to get to the desired position.
		this->commands += zpr::sprint(" 1 0 0 1 0 0 Tm {} {} Td\n", pos.x, pos.y);
		this->commands += "[";
	}

	void Text::offset(Vector offset)
	{
		// again, close the current group, call the TJ operator, and start a new group
		this->commands += zpr::sprint("] TJ {} {} Td\n", offset.x, offset.y);
		this->commands += "[";
	}

	void Text::beginGroup()
	{
		this->commands += "] TJ\n[";
	}

	void Text::offset(Scalar ofs)
	{
		// note that we specify that a positive offset moves the glyph to the right
		this->commands += zpr::sprint(" {}", -1 * ofs.x);
	}

	void Text::addEncoded(size_t bytes, uint32_t encodedValue)
	{
		if(bytes == 1)
			this->commands += zpr::sprint("<{02x}>", encodedValue & 0xFF);
		else if(bytes == 2)
			this->commands += zpr::sprint("<{04x}>", encodedValue & 0xFFFF);
		else if(bytes == 4)
			this->commands += zpr::sprint("<{08x}>", encodedValue & 0xFFFF'FFFF);
		else
			pdf::error("invalid number of bytes");
	}



#if 0
	void Text::addText(zst::str_view text)
	{
		// if(this->in_group)
		// 	this->commands += zpr::sprint(" {}", encode_text_with_font(this->font, text));
		// else
		// 	this->commands += zpr::sprint(" [{}] TJ", encode_text_with_font(this->font, text));
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
#endif
}
