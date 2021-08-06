// features.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <map>
#include <utility>

#include "font/tag.h"

namespace font
{
	struct Table;
	struct FontFile;

	struct TaggedTable
	{
		Tag tag;
		size_t file_offset;
	};

	struct GlyphAdjustment
	{
		int16_t horz_placement;
		int16_t vert_placement;
		int16_t horz_advance;
		int16_t vert_advance;
	};

	struct LookupTable
	{
		bool present;
		uint16_t type;
		size_t file_offset;
	};

	constexpr uint16_t GPOS_LOOKUP_SINGLE           = 1;
	constexpr uint16_t GPOS_LOOKUP_PAIR             = 2;
	constexpr uint16_t GPOS_LOOKUP_CURSIVE          = 3;
	constexpr uint16_t GPOS_LOOKUP_MARK_TO_BASE     = 4;
	constexpr uint16_t GPOS_LOOKUP_MARK_TO_LIGA     = 5;
	constexpr uint16_t GPOS_LOOKUP_MARK_TO_MARK     = 6;
	constexpr uint16_t GPOS_LOOKUP_CONTEXT_POS      = 7;
	constexpr uint16_t GPOS_LOOKUP_CHAINED_CONTEXT  = 8;
	constexpr uint16_t GPOS_LOOKUP_EXTENSION_POS    = 9;
	constexpr uint16_t GPOS_LOOKUP_MAX              = 10;

	struct GPosTable
	{
		// indexed by the GPOS_LOOKUP enumeration.
		LookupTable lookup_tables[GPOS_LOOKUP_MAX];
	};

	std::vector<TaggedTable> parseTaggedList(FontFile* font, zst::byte_span list);
	int getGlyphClass(zst::byte_span classdef_table, uint32_t glyphId);

	void parseGPOS(FontFile* font, const Table& gpos_table);
}
