// gpos.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cassert>

#include "util.h"
#include "error.h"

#include "font/font.h"
#include "font/features.h"


namespace font::off
{
	static void combine_adjustments(GlyphAdjustment& a, const GlyphAdjustment& b)
	{
		a.horz_placement += b.horz_placement;
		a.vert_placement += b.vert_placement;
		a.horz_advance += b.horz_advance;
		a.vert_advance += b.vert_advance;
	}

	std::map<size_t, GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(FontFile* font,
		zst::span<uint32_t> glyphs, const FeatureSet& features)
	{
		auto gpos = font->gpos_table;

		/*
			OFF 1.9, page 217

			During text processing, a client applies a lookup to each glyph in the string before moving
			to the next lookup. A lookup is finished for a glyph after the client locates the target glyph
			or glyph context and performs a positioning action, if specified. To move to the "next" glyph,
			the client will typically skip all the glyphs that participated in the lookup operation: glyphs
			that were positioned as well as any other glyphs that formed a context for the operation.

			There is just one exception: the "next" glyph in a sequence may be one of those that formed a
			context for the operation just performed. For example, in the case of pair positioning operations
			(i.e., kerning), if the ValueRecord for the second glyph is null, that glyph is treated as the
			"next" glyph in the sequence.
			-----------------------------

			TL;DR: for(lookups) { for(glyphs) { ... } }, and *NOT* the transposed.
		*/

		std::map<size_t, GlyphAdjustment> adjustments {};
		auto lookups = getLookupTablesForFeatures(font->gpos_table, features);

		for(auto& lookup_idx : lookups)
		{
			assert(lookup_idx < gpos.lookups.size());
			auto& lookup = gpos.lookups[lookup_idx];

			for(size_t i = 0; i < glyphs.size(); i++)
			{
				if(lookup.type == gpos::LOOKUP_SINGLE)
				{
					if(auto adj = gpos::lookupSingleAdjustment(lookup, glyphs[i]); adj.has_value())
						combine_adjustments(adjustments[i], *adj);
				}
				else if((lookup.type == gpos::LOOKUP_PAIR) && (i + 1 < glyphs.size()))
				{
					auto [ a1, a2 ] = gpos::lookupPairAdjustment(lookup, glyphs[i], glyphs[i + 1]);
					if(a1.has_value())
						combine_adjustments(adjustments[i], *a1);

					if(a2.has_value())
					{
						// if the second adjustment was not null, then we skip the second glyph
						combine_adjustments(adjustments[i + 1], *a2);
						i += 1;
					}
				}
			}
		}

		return adjustments;
	}
}
