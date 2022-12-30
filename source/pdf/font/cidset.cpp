// cidset.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "types.h" // for GlyphId

#include "pdf/font.h"   // for Font, File
#include "pdf/object.h" // for Stream

#include "font/font.h" // for FontFile

namespace pdf
{
	void Font::writeCIDSet(File* doc) const
	{
		auto stream = this->cidset;
		stream->clear();

		assert(this->source_file != nullptr);

		int num_bits = 0;
		uint8_t current = 0;
		for(uint32_t gid = 0; gid < this->source_file->num_glyphs; gid++)
		{
			bool bit = m_used_glyphs.find(GlyphId { gid }) != m_used_glyphs.end();
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
