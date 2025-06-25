// cidset.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "types.h"

#include "pdf/font.h"
#include "pdf/object.h"

#include "font/font_file.h"

namespace pdf
{
	void PdfFont::writeCIDSet() const
	{
		auto font_file = dynamic_cast<const font::FontFile*>(m_source.get());
		if(not font_file)
			return;

		auto stream = m_cidset;
		stream->clear();

		/*
		    NOTE: PDF/A Specification: ISO 19005-2:2011, Clause: 6.2.11.4.2, Test number: 2

		    If the FontDescriptor dictionary of an embedded CID font contains a CIDSet stream, then it shall
		    identify all CIDs which are present in the font program, regardless of whether a CID in the font is
		    referenced or used by the PDF or not.


		    Our current subsetting strategy for both TTF and CFF fonts is simple to delete the glyph
		    info for unused glyphs, and *NOT* to re-number the glyphs in any way. This means that the number
		    of glyphs "present" in the font does not change --- ie. we must mark every glyph as present.
		*/

		uint8_t ff[1] = { 0xFF };
		for(size_t i = 0, n = (font_file->numGlyphs() + 7) / 8; i < n; i++)
			stream->append(ff, 1);

#if 0
		int num_bits = 0;
		uint8_t current = 0;
		for(uint32_t gid = 0; gid < font_file->numGlyphs(); gid++)
		{
			bool bit = true; // font_file->isGlyphUsed(GlyphId { gid });
			current <<= 1;
			current |= (bit ? 1 : 0);
			num_bits++;

			if(num_bits == 8)
			{
				stream->append(&current, 1);
				num_bits = 0;
				current = 0;
			}
		}

		if(num_bits > 0)
		{
			current <<= (8 - num_bits);
			stream->append(&current, 1);
		}
#endif
	}
}
