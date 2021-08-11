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

	/*
		This always uses the pdf 'TJ' operator. when an offset (or moveAbs) is made,
		the prior TJ operator is concluded and a new one is created.

		While we have `beginGroup()`, there is no `endGroup()` equivalent, since there
		is no nesting of text groups. Note that 'group' here just refers to one invocation
		of the 'TJ' operator, eg. "[(hello) 10 (world)] TJ". Starting a new group with
		`beginGroup()` implicitly ends the previous group, and calling serialise() implicitly
		ends the group.

		Text starts with a group already "open", so it is usually unnecessary to call `beginGroup`
		manually. Furthermore, (as mentioned above), offset(Vector) and moveAbs(Vector) will
		perform group opening and closing automatically.
	*/
	struct Text : PageObject
	{
		virtual std::string serialise(const Page* page) const override;

		// must be called before the first text item is inserted (addText).
		void setFont(const Font* font, Scalar height);

		void moveAbs(Vector pos);
		void offset(Vector offset);

		// closes the current TJ group, inserts the pdf command *verbatim* into the command stream,
		// and opens a new group; nothing else is done (and the TJ groups don't affect the
		// graphic state).
		void insertPDFCommand(zst::str_view sv);

		// begin a new group. a `Text` starts with a group already open, so it is usually
		// not necessary to call this function directly.
		void beginGroup();

		// this offsets the next set of glyph ids (or whatever encoding scheme) that
		// goes into the 'TJ' operator. in horizontal writing mode, a positive value
		// moves the next glyph TO THE RIGHT. in vertical writing mode, a positive
		// value moves the next glyph DOWNWARDS.
		void offset(Scalar ofs);

		// add an encoded value to the text stream. for now, this is a glyph ID, but
		// in the future we might use cmaps so this might be a unicode codepoint, or
		// whatever. `bytes` indicates how many bytes the value should be printed as;
		// 2 bytes gives us <AABB>, 4 gives <AABBCCDD>, etc.
		void addEncoded(size_t bytes, uint32_t encodedValue);

	private:
		std::string commands;
		std::vector<const Font*> used_fonts;
	};
}
