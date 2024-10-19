// text.cpp
// Copyright (c) 2021, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <utf8proc/utf8proc.h>

#include "pdf/font.h"
#include "pdf/misc.h"
#include "pdf/page.h"
#include "pdf/text.h"
#include "pdf/units.h"

namespace pdf
{
	void Text::writePdfCommands(Stream* stream) const
	{
		auto str_buf = zst::buffer<char>();
		auto appender = [&str_buf](const char* c, size_t n) { str_buf.append(c, n); };

		zpr::cprint(appender, "q BT\n");
		for(size_t i = 0; i < m_groups.size(); i++)
		{
			auto& group = m_groups[i];

			for(auto& cmd : group.commands)
				appender(cmd.data(), cmd.size());

			// the last group won't have a closing ')' if it's unicode
			if(not group.text.empty())
			{
				zpr::cprint(appender, "[{}{}] TJ\n", group.text,
				    group.is_unicode && i + 1 == m_groups.size() ? ")" : "");
			}
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

	void Text::setColour(const sap::Colour& colour)
	{
		if(m_current_colour == colour)
			return;

		m_current_colour = colour;
		if(m_current_colour.isRGB())
		{
			auto rgb = m_current_colour.rgb();
			this->insertPDFCommand(zpr::sprint(" {} {} {} rg\n", rgb.r, rgb.g, rgb.b));
		}
		else if(m_current_colour.isCMYK())
		{
			auto cmyk = m_current_colour.cmyk();
			this->insertPDFCommand(zpr::sprint(" {} {} {} {} k\n", cmyk.c, cmyk.m, cmyk.y, cmyk.k));
		}
		else
		{
			sap::internal_error("invalid colour type");
		}
	}

	PdfScalar Text::currentFontSize() const
	{
		return m_current_font.height;
	}

	void Text::insertPDFCommand(zst::str_view sv)
	{
		// if we have no groups yet, *OR* the last group has text in it, we must create a new group.
		if(m_groups.empty() || !m_groups.back().text.empty())
		{
			if(not m_groups.empty() && m_groups.back().is_unicode && not m_groups.back().text.empty())
				m_groups.back().text += ")";

			m_groups.emplace_back();
		}

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

	void Text::rise(PdfScalar rise)
	{
		this->insertPDFCommand(zpr::sprint(" {} Ts", rise.value()));
	}

	void Text::offset(TextSpace1d ofs)
	{
		if(ofs.iszero())
			return;

		Group* grp = nullptr;
		if(m_groups.empty())
			grp = &m_groups.emplace_back();
		else
			grp = &m_groups.back();

		// note that we specify that a positive offset moves the glyph to the right
		if(grp->is_unicode && not grp->text.empty())
			grp->text += ")";

		grp->text += zpr::sprint(" {} ", -1 * ofs.value());

		if(grp->is_unicode)
			grp->text += "(";
	}

	void Text::addEncoded(size_t bytes, uint32_t encodedValue)
	{
		if(m_groups.empty())
		{
			m_groups.emplace_back();
		}
		else if(m_groups.back().is_unicode)
		{
			m_groups.back().text += ")";
			m_groups.emplace_back();
		}

		if(bytes == 1)
			m_groups.back().text += zpr::sprint("<{02x}>", encodedValue & 0xFF);
		else if(bytes == 2)
			m_groups.back().text += zpr::sprint("<{04x}>", encodedValue & 0xFFFF);
		else if(bytes == 4)
			m_groups.back().text += zpr::sprint("<{08x}>", encodedValue & 0xFFFF'FFFF);
		else
			pdf::error("invalid number of bytes");
	}

	void Text::addUnicodeText(zst::wstr_view text)
	{
		Group* grp = nullptr;
		if(m_groups.empty() || not m_groups.back().is_unicode)
		{
			grp = &m_groups.emplace_back();
			grp->is_unicode = true;
			grp->text += "(";
		}
		else
		{
			grp = &m_groups.back();
		}

		for(auto& cp : text)
		{
			if(cp == U'(')
			{
				grp->text += "\\(";
			}
			else if(cp == U')')
			{
				grp->text += "\\)";
			}
			else if(cp == U'\\')
			{
				grp->text += "\\\\";
			}
			else
			{
				if(m_current_font.font->isCIDFont())
				{
					uint8_t tmp[4] {};
					auto n = utf8proc_encode_char(static_cast<int32_t>(cp), &tmp[0]);
					grp->text += zst::byte_span(&tmp[0], static_cast<size_t>(n)).chars().sv();
				}
				else
				{
					grp->text += static_cast<char>(cp);
				}
			}
		}
	}
}
