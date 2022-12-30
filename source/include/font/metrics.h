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

		int hhea_ascent;
		int hhea_descent;
		int hhea_linegap;
		int units_per_em;

		int x_height;
		int cap_height;

		int typo_ascent;
		int typo_descent;
		int typo_linegap;

		FontScalar default_line_spacing;

		bool is_monospaced;
		double italic_angle;
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
		FontScalar horz_placement;
		FontScalar vert_placement;
		FontScalar horz_advance;
		FontScalar vert_advance;
	};

	struct GlyphInfo
	{
		GlyphId gid;
		GlyphMetrics metrics;
		GlyphAdjustment adjustments;
	};
}
