// text.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/font.h"  // for Font
#include "pdf/misc.h"  // for error
#include "pdf/page.h"  // for Page
#include "pdf/text.h"  // for Text::Group, Text, Text::(anonymous)
#include "pdf/units.h" // for Offset2d, Position2d, TextSpace1d, PdfScalar

namespace pdf
{
	void Text::writePdfCommands(Stream* stream) const
	{
		auto str_buf = zst::buffer<char>();
		auto appender = [&str_buf](const char* c, size_t n) { str_buf.append(c, n); };

		zpr::cprint(appender, "q BT\n");
		for(auto& group : m_groups)
		{
			for(auto& cmd : group.commands)
				appender(cmd.data(), cmd.size());

			zpr::cprint(appender, "[{}] TJ\n", group.text);
		}

		zpr::cprint(appender, "ET Q\n");
		stream->append(str_buf.bytes());
	}

	void Text::addResources(const Page* page) const
	{
		for(auto f : m_used_fonts)
			page->addResource(f);
	}

	void Text::setFont(const PdfFont* font, PdfScalar height)
	{
		assert(font != nullptr);

		if(m_current_font.font == font && m_current_font.height == height)
			return;

		m_used_fonts.insert(font);
		this->insertPDFCommand(zpr::sprint(" /{} {} Tf\n", font->resourceName(), height));

		m_current_font.font = font;
		m_current_font.height = height;
	}

	PdfScalar Text::currentFontSize() const
	{
		return m_current_font.height;
	}

	void Text::insertPDFCommand(zst::str_view sv)
	{
		// if we have no groups yet, *OR* the last group has text in it, we must create a new group.
		if(m_groups.empty() || !m_groups.back().text.empty())
			m_groups.emplace_back();

		m_groups.back().commands.push_back(sv.str());
	}


	// convention is that all appends to the command list should start with a " ".
	void Text::moveAbs(Position2d pos)
	{
		// do this in two steps; first, replace the text matrix with the identity to get to (0, 0),
		// then perform an offset to get to the desired position.
		this->insertPDFCommand(zpr::sprint(" 1 0 0 1 0 0 Tm {} {} Td\n", pos.x(), pos.y()));
	}

	void Text::nextLine(Offset2d offset)
	{
		this->insertPDFCommand(zpr::sprint(" {} {} Td\n", offset.x(), offset.y()));
	}

	void Text::rise(TextSpace1d rise)
	{
		// positive is up
		this->insertPDFCommand(zpr::sprint(" {} Ts", rise.value()));
	}

	void Text::offset(TextSpace1d ofs)
	{
		if(ofs.iszero())
			return;

		if(m_groups.empty())
			m_groups.emplace_back();

		// note that we specify that a positive offset moves the glyph to the right
		m_groups.back().text += zpr::sprint(" {} ", -1 * ofs.value());
	}

	void Text::addEncoded(size_t bytes, uint32_t encodedValue)
	{
		if(m_groups.empty())
			m_groups.emplace_back();

		if(bytes == 1)
			m_groups.back().text += zpr::sprint("<{02x}>", encodedValue & 0xFF);
		else if(bytes == 2)
			m_groups.back().text += zpr::sprint("<{04x}>", encodedValue & 0xFFFF);
		else if(bytes == 4)
			m_groups.back().text += zpr::sprint("<{08x}>", encodedValue & 0xFFFF'FFFF);
		else
			pdf::error("invalid number of bytes");
	}
}
