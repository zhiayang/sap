// cidset.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "types.h" // for GlyphId

#include "pdf/font.h"   // for Font, File
#include "pdf/object.h" // for Stream

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

		int num_bits = 0;
		uint8_t current = 0;
		for(uint32_t gid = 0; gid < font_file->numGlyphs(); gid++)
		{
			bool bit = font_file->isGlyphUsed(GlyphId { gid });
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
	}
}
