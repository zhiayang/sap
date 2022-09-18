// text.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <set>

#include "pdf/units.h"
#include "pdf/pageobject.h"

namespace pdf
{
	struct Font;
	struct Page;

	/*
	    The Text object operates on the idea of 'groups' (but these groups are hidden from the caller).
	    A group is simply an instance of the 'TJ' pdf operator, containing one or more glyph ids
	    to draw, as well as any number of horizontal adjustment commands within the array.

	    Between groups, arbitrary PDF commands can be inserted; these are usually either relative or
	    absolute positioning commands (Td, or Tm+Td respectively), or font-changing commands (Tf).

	    Groups are managed automatically, and there is not facility to control how groups are written.
	    Performing a non-text-related command (eg. positioning commands, font changes) will end the current
	    group.
	*/
	struct Text : PageObject
	{
		virtual std::string serialise(const Page* page) const override;

		// must be called before the first text item is inserted (addText).
		void setFont(const Font* font, Scalar height);

		void moveAbs(Position2d pos);
		void nextLine(Offset2d offset);

		/*
		    closes the current TJ group, inserts the pdf command *verbatim* into the command stream,
		    and opens a new group; nothing else is done (and the TJ groups don't affect the
		    graphic state).
		*/
		void insertPDFCommand(zst::str_view sv);

		/*
		    this offsets the next set of glyph ids (or whatever encoding scheme) that
		    goes into the 'TJ' operator. in horizontal writing mode, a positive value
		    moves the next glyph TO THE RIGHT. in vertical writing mode, a positive
		    value moves the next glyph DOWNWARDS.
		*/
		void offset(Scalar ofs);

		/*
		    add an encoded value to the text stream. for now, this is a glyph ID, but
		    in the future we might use cmaps so this might be a unicode codepoint, or
		    whatever. `bytes` indicates how many bytes the value should be printed as;
		    2 bytes gives us <AABB>, 4 gives <AABBCCDD>, etc.
		*/
		void addEncoded(size_t bytes, uint32_t encodedValue);

	private:
		/*
		    a Group is defined here as an instance of a TJ operator. we attach commands to groups,
		    and the command is defined to be written *before* the group content itself.
		*/
		struct Group
		{
			std::vector<std::string> commands;
			std::string text;
		};

		struct
		{
			const Font* font = nullptr;
			Scalar height {};
		} m_current_font {};

		std::vector<Group> m_groups {};
		std::set<const Font*> m_used_fonts {};
	};
}
