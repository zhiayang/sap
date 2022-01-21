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

	struct GlyphAdjustment
	{
		int16_t horz_placement;
		int16_t vert_placement;
		int16_t horz_advance;
		int16_t vert_advance;
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
}

namespace font::off
{
	int getGlyphClass(zst::byte_span classdef_table, uint32_t glyphId);

	// returns a map from classId -> [ glyphIds ], for all glyphs that have a class
	// defined (ie. that are not implicitly class 0)
	std::map<int, std::vector<uint32_t>> parseClassDefTable(zst::byte_span classdef_table);

	std::optional<int> getGlyphCoverageIndex(zst::byte_span coverage_table, uint32_t glyphId);

	// returns a map from coverageIndex -> glyphId, for every glyph in the coverage table.
	std::map<int, uint32_t> parseCoverageTable(zst::byte_span coverage_table);


	/*
		From the OFF specification:

		1. get the current script (latin, cyrillic, etc.), and lookup the Script struct
		2. get the current language (eg. english, greek) and lookup the Language struct
			(within the Script)

		3. for the current language, get the required feature (if any) and any optional
			features. Intersect this with the enabled features from the provided FeatureSet

		4. each feature has some number (possibly 0?) of lookups; visit the features and
			lookups in order, and apply (all) the lookups that match.

		This is the same set of steps for both GSUB and GPOS (but we apply gsub first).

		For steps (1) and (2), if the current script is unknown (or the font does not
		contain an entry for that script), use `DFLT`, and similar for the language.
	*/
	struct Language
	{
		Tag tag;
		std::optional<uint16_t> required_feature;
		std::vector<uint16_t> features;
	};

	struct Script
	{
		Tag tag;
		std::map<Tag, Language> languages;
	};

	struct Feature
	{
		Tag tag;
		std::optional<zst::byte_span> parameters_table;
		std::vector<uint16_t> lookups;
	};

	struct LookupTable
	{
		uint16_t type;
		uint16_t flags;
		uint16_t mark_filtering_set;
		std::vector<zst::byte_span> subtables;
	};

	struct GPosTable
	{
		std::vector<Feature> features;
		std::map<Tag, Script> scripts;
		std::vector<LookupTable> lookups;

		std::optional<zst::byte_span> feature_variations_table;
	};


	struct GSubTable
	{
		std::vector<Feature> features;
		std::map<Tag, Script> scripts;
		std::vector<LookupTable> lookups;

		std::optional<zst::byte_span> feature_variations_table;
	};





	struct FeatureSet
	{
		Tag script;
		Tag language;
		std::set<Tag> enabled_features {};
	};

	/*
		Using the GPOS table, try to look for positioning adjustments for the input glyph
		sequence, using the enabled features in the `feature_set`.

		The return type is a map of glyph index (of the input sequence) to a GlyphAdjustment,
		if that particular glyph needs to be adjusted. Only glyphs that need to be adjusted
		will be part of the map.
	*/
	std::map<size_t, GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(FontFile* font,
		zst::span<uint32_t> glyphs, const FeatureSet& features);


	/*
		Parse the GPOS and GSUB tables from the OTF top-level Table.
	*/
	void parseGPos(FontFile* font, const Table& gpos_table);
	void parseGSub(FontFile* font, const Table& gsub_table);



	/*
		private, implementation stuff
		=============================
	*/

	struct TaggedTable
	{
		Tag tag;
		zst::byte_span data;
	};


	/*
		Return a list of LookupTable indices for the given feature set in either the GPOS or GSUB tables.
	*/
	template <typename TableKind>
	std::vector<uint16_t> getLookupTablesForFeatures(TableKind& gpos_or_gsub, const FeatureSet& features);

	/*
		Parse the script and language tables from the given buffer.
	*/
	std::map<Tag, Script> parseScriptAndLanguageTables(zst::byte_span buf);

	/*
		Parse the FeatureList.
	*/
	std::vector<Feature> parseFeatureList(zst::byte_span buf);

	/*
		Parse the LookupList.
	*/
	std::vector<LookupTable> parseLookupList(zst::byte_span buf);

	/*
		Parse a "Tagged List" -- with the following layout:

		u16 count;
		Record records[count];

		Record:
			u32 Tag
			u16 offset_from_start_of_table
	*/
	std::vector<TaggedTable> parseTaggedList(zst::byte_span buf);
}


// declares GPOS-specific lookup functions
namespace font::off::gpos
{
	constexpr uint16_t LOOKUP_SINGLE            = 1;
	constexpr uint16_t LOOKUP_PAIR              = 2;
	constexpr uint16_t LOOKUP_CURSIVE           = 3;
	constexpr uint16_t LOOKUP_MARK_TO_BASE      = 4;
	constexpr uint16_t LOOKUP_MARK_TO_LIGA      = 5;
	constexpr uint16_t LOOKUP_MARK_TO_MARK      = 6;
	constexpr uint16_t LOOKUP_CONTEXT_POS       = 7;
	constexpr uint16_t LOOKUP_CHAINED_CONTEXT   = 8;
	constexpr uint16_t LOOKUP_EXTENSION_POS     = 9;
	constexpr uint16_t LOOKUP_MAX               = 10;

	using OptionalGA = std::optional<GlyphAdjustment>;

	OptionalGA lookupSingleAdjustment(LookupTable& lookup, uint32_t gid);

	/*
		It is important to know whether there was an adjustment for the second glyph in the pair
		or not; see OFF 1.9, page 220:

		If valueFormat2 is set to 0, then the second glyph of the pair is the "next" glyph
		for which a lookup should be performed.

		So, instead of returning an optional of pair, we return a pair of optionals.
	*/
	std::pair<OptionalGA, OptionalGA> lookupPairAdjustment(LookupTable& lookup, uint32_t gid1, uint32_t gid2);
}

// declares GSUB-specific lookup functions
namespace font::off::gsub
{
	constexpr uint16_t LOOKUP_SINGLE            = 1;
	constexpr uint16_t LOOKUP_MULTIPLE          = 2;
	constexpr uint16_t LOOKUP_ALTERNATE         = 3;
	constexpr uint16_t LOOKUP_LIGATURE          = 4;
	constexpr uint16_t LOOKUP_CONTEXT           = 5;
	constexpr uint16_t LOOKUP_CHAINED_CONTEXT   = 6;
	constexpr uint16_t LOOKUP_EXTENSION_SUBST   = 7;
	constexpr uint16_t LOOKUP_REVERSE_CHAIN     = 8;
	constexpr uint16_t LOOKUP_MAX               = 9;
}


namespace font::off
{
	/*
		GPOS FEATURES
		=============
	*/
	namespace feature
	{
		/*
			kerning: self explanatory
		*/
		constexpr auto kern = Tag("kern");

		/*
			capital spacing: for all-caps words, this usually adjusts the glyph advance to be
			slightly larger, so the glyphs are not so close together.
		*/
		constexpr auto cpsp = Tag("cpsp");
	}


	/*
		GSUB FEATURES
		=============
	*/
	namespace feature
	{
	}
}

