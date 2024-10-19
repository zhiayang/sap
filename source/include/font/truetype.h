// truetype.h
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "types.h" // for GlyphId

namespace font
{
	struct FontFile;
	struct GlyphMetrics;
}

namespace font::truetype
{
	struct Glyph
	{
		uint16_t gid = 0;
		std::set<uint16_t> component_gids {};

		zst::byte_span glyph_data {};
	};

	struct TTData
	{
		size_t loca_bytes_per_entry = 0;

		std::vector<Glyph> glyphs {};
		zst::byte_span glyf_data {};
	};

	struct BoundingBox
	{
		double xmin;
		double ymin;
		double xmax;
		double ymax;
	};

	struct TTSubset
	{
		zst::byte_buffer loca_table {};
		zst::byte_buffer glyf_table {};
	};

	/*
	    Return the bounding box data for the given glyph id by inspecting its glyf data.
	*/
	BoundingBox getGlyphBoundingBox(TTData* tt, GlyphId glyph_id);
}
