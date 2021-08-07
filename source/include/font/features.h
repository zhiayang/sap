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

	constexpr uint16_t GSUB_LOOKUP_SINGLE           = 1;
	constexpr uint16_t GSUB_LOOKUP_MULTIPLE         = 2;
	constexpr uint16_t GSUB_LOOKUP_ALTERNATE        = 3;
	constexpr uint16_t GSUB_LOOKUP_LIGATURE         = 4;
	constexpr uint16_t GSUB_LOOKUP_CONTEXT          = 5;
	constexpr uint16_t GSUB_LOOKUP_CHAINED_CONTEXT  = 6;
	constexpr uint16_t GSUB_LOOKUP_EXTENSION_SUBST  = 7;
	constexpr uint16_t GSUB_LOOKUP_REVERSE_CHAIN    = 8;
	constexpr uint16_t GSUB_LOOKUP_MAX              = 9;

	struct GSubTable
	{
		// indexed by the GPOS_LOOKUP enumeration.
		LookupTable lookup_tables[GSUB_LOOKUP_MAX];
	};


	constexpr size_t MAX_LIGATURE_LENGTH = 4;
	struct GlyphLigature
	{
		uint32_t num_glyphs;
		uint32_t glyphs[MAX_LIGATURE_LENGTH];
		uint32_t substitute;
	};

	struct GlyphLigatureSet
	{
		std::vector<GlyphLigature> ligatures;
	};


	std::vector<TaggedTable> parseTaggedList(FontFile* font, zst::byte_span list);
	LookupTable parseLookupTable(FontFile* font, zst::byte_span buf);

	int getGlyphClass(zst::byte_span classdef_table, uint32_t glyphId);

	std::optional<int> getGlyphCoverageIndex(zst::byte_span coverage_table, uint32_t glyphId);

	// returns a map from coverageIndex -> glyphId, for every glyph in the coverage table.
	std::map<int, uint32_t> parseCoverageTable(zst::byte_span coverage_table);

	void parseGPos(FontFile* font, const Table& gpos_table);
	void parseGSub(FontFile* font, const Table& gsub_table);

}
