// cidset.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/font.h"
#include "pdf/object.h"

namespace pdf
{
	void Font::writeCIDSet(Document* doc) const
	{
		auto stream = this->cidset;
		assert(this->source_file != nullptr);

		int num_bits = 0;
		uint8_t current = 0;
		for(size_t gid = 0; gid < this->source_file->num_glyphs; gid++)
		{
			bool bit = this->glyph_metrics.find(gid) != this->glyph_metrics.end();
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
