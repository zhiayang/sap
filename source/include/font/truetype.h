// truetype.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <zst.h>
#include <utility>

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
		Parse the `loca` table from the given buffer, filling the information into the TTData struct.
	*/
	void parseLocaTable(FontFile* font, zst::byte_span loca_table);

	/*
		Parse the `glyf` table from the given buffer, filling the information into the TTData struct.
	*/
	void parseGlyfTable(FontFile* font, zst::byte_span glyf_table);

	/*
		Return the bounding box data for the given glyph id by inspecting its glyf data.
	*/
	BoundingBox getGlyphBoundingBox(TTData* tt, uint32_t glyph_id);

	/*
		Subset the glyphs in the original loca/glyf tables based on the provided `used_glyphs`.
	*/
	TTSubset createTTSubset(FontFile* font, const std::map<uint32_t, GlyphMetrics>& used_glyphs);
}