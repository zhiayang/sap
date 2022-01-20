// features.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

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
		uint16_t type;
		uint16_t flags;
		uint16_t mark_filtering_set;

		// note: data includes the header, and offsets also include it
		// ie. the first item will be at offset ≠ 0, and data[0] is *not* the first item.
		// note also that the data span is *NOT* bounded! it will go to the end of the file.
		std::vector<uint16_t> subtable_offsets;
		zst::byte_span data;
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
		std::vector<LookupTable> lookup_tables;
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
		std::vector<LookupTable> lookup_tables[GSUB_LOOKUP_MAX];
	};


	constexpr size_t MAX_LIGATURE_LENGTH = 8;
	struct GlyphLigature
	{
		uint32_t num_glyphs;
		uint32_t glyphs[MAX_LIGATURE_LENGTH];
		uint32_t substitute;

		bool operator== (const GlyphLigature& gl) const
		{
			return memcmp(this, &gl, sizeof(GlyphLigature)) == 0;
		}
	};

	// honestly, this struct shouldn't have padding.
	static_assert(sizeof(GlyphLigature) == (2 + MAX_LIGATURE_LENGTH) * sizeof(uint32_t));

	struct GlyphLigatureSet
	{
		std::vector<GlyphLigature> ligatures;

		bool operator== (const GlyphLigatureSet& gls) const { return this->ligatures == gls.ligatures; }
	};


	std::vector<TaggedTable> parseTaggedList(FontFile* font, zst::byte_span list);
	std::vector<LookupTable> parseLookupList(FontFile* font, zst::byte_span buf);

	int getGlyphClass(zst::byte_span classdef_table, uint32_t glyphId);

	// returns a map from classId -> [ glyphIds ], for all glyphs that have a class
	// defined (ie. that are not implicitly class 0)
	std::map<int, std::vector<uint32_t>> parseClassDefTable(zst::byte_span classdef_table);

	std::optional<int> getGlyphCoverageIndex(zst::byte_span coverage_table, uint32_t glyphId);

	// returns a map from coverageIndex -> glyphId, for every glyph in the coverage table.
	std::map<int, uint32_t> parseCoverageTable(zst::byte_span coverage_table);

	void parseGPos(FontFile* font, const Table& gpos_table);
	void parseGSub(FontFile* font, const Table& gsub_table);

}
