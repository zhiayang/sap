// metrics.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "types.h"

#include "font/font_scalar.h"

namespace font
{
	struct FontMetrics
	{
		int xmin;
		int ymin;
		int xmax;
		int ymax;

		FontScalar hhea_ascent;
		FontScalar hhea_descent;
		FontScalar hhea_linegap;
		int units_per_em;

		FontScalar x_height;
		FontScalar cap_height;

		FontScalar typo_ascent;
		FontScalar typo_descent;
		FontScalar typo_linegap;

		FontScalar default_line_spacing;

		bool is_monospaced;
		double italic_angle;

		int stem_v;
		int stem_h;
	};

	struct GlyphMetrics
	{
		FontScalar horz_advance;
		FontScalar vert_advance;

		FontScalar horz_placement;
		FontScalar vert_placement;

		FontScalar xmin;
		FontScalar xmax;
		FontScalar ymin;
		FontScalar ymax;

		FontScalar left_side_bearing;
		FontScalar right_side_bearing;
	};

	struct GlyphAdjustment
	{
		FontScalar horz_placement = 0;
		FontScalar vert_placement = 0;
		FontScalar horz_advance = 0;
		FontScalar vert_advance = 0;
	};

	struct GlyphInfo
	{
		GlyphId gid;
		GlyphMetrics metrics;
		GlyphAdjustment adjustments;
	};

	/*
	    The result of performing (all required) glyph substitutions on a glyph string; contains
	    the list of output glyph ids.

	    There are also 3 maps present. For all of them, the key is always the *output* glyph, and
	    the value is the *input* glyph.

	    1. replacements -- a simple one->one mapping from output glyph to the original input glyph.
	    2. contractions -- a one->many mapping from output glyph to the original input glyphs (ligatures)
	    3. extra_glyphs -- see the long comment below on the one-to-many case.

	    The intent of this is so that we can map the correct unicode codepoints to any substituted glyphs. For
	    one-to-one mappings it's simple -- the output glyph has the same codepoint as the input glyph.

	    For many-to-one it's also simple -- the output glyph is logically composed of the given list of inputs, so
	    it represents a list of codepoints.

	    The one-to-many case is tricky. For now, we make the assumption that the output glyphs are all present
	    in the font's cmap. Otherwise, there is no way for us to know how to decompose a single glyph's codepoint
	    into multiple codepoints. To account for this (ie. ensure that the cmap is emitted), we the last list
	    is just a list of extra glyph ids to include.
	*/
	struct SubstitutionMapping
	{
		std::set<GlyphId> extra_glyphs;
		std::map<GlyphId, GlyphId> replacements;
		std::map<GlyphId, std::vector<GlyphId>> contractions;
	};

	struct SubstitutedGlyphString
	{
		std::vector<GlyphId> glyphs;
		SubstitutionMapping mapping;
	};
}
