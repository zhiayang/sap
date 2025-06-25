// aat.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <variant>

#include "util.h"
#include "types.h"

#include "font/tag.h"
#include "font/metrics.h"
#include "font/features.h"
#include "font/font_scalar.h"

namespace font
{
	struct Table;
	struct FontFile;
}

// stuff dealing with Apple Advanced Typography tables
// (basically apple's own version of GPOS, GSUB, GDEF, etc.)

namespace font::aat
{
	struct Feature
	{
		uint16_t type;
		uint16_t selector;

		constexpr bool operator==(Feature f) const { return type == f.type && selector == f.selector; }
		constexpr bool operator!=(Feature f) const { return !(*this == f); }
	};

	/*
	    lookup tables
	*/
	struct Lookup
	{
		util::hashmap<GlyphId, uint64_t> map;
	};

	std::optional<Lookup> parseLookupTable(zst::byte_span buf, size_t num_font_glyphs);
	std::optional<uint64_t> searchLookupTable(zst::byte_span table, size_t num_font_glyphs, GlyphId target);

	/*
	    encodes a state machine
	*/
	struct StateTable
	{
		size_t num_classes;
		util::hashmap<GlyphId, uint16_t> glyph_classes;

		zst::byte_span state_array;
		size_t state_row_size;

		zst::byte_span entry_array;
		bool is_extended;
	};

	StateTable parseStateTable(zst::byte_span& state_table, size_t num_font_glyphs);
	StateTable parseExtendedStateTable(zst::byte_span& state_table, size_t num_font_glyphs);







	struct KernSubTableCoverage
	{
		bool is_vertical;
		bool is_cross_stream;
		bool is_variation;
		bool is_override;
		bool is_minimum;
	};

	struct KernSubTable0
	{
		struct Pair
		{
			GlyphId left;
			GlyphId right;
			FontScalar shift;
		};

		KernSubTableCoverage coverage;
		size_t num_pairs;

		zst::byte_span lookup_table;
	};

	struct KernSubTable1
	{
		KernSubTableCoverage coverage;
	};

	struct KernSubTable2
	{
		KernSubTableCoverage coverage;

		util::hashmap<GlyphId, uint16_t> left_glyph_classes;
		util::hashmap<GlyphId, uint16_t> right_glyph_classes;

		zst::byte_span lookup_array;
	};

	struct KernSubTable3
	{
		zst::byte_span table;
		KernSubTableCoverage coverage;
	};

	struct KernTable
	{
		std::vector<KernSubTable0> subtables_f0;
		std::vector<KernSubTable1> subtables_f1;
		std::vector<KernSubTable2> subtables_f2;
		std::vector<KernSubTable3> subtables_f3;
	};

	util::hashmap<size_t, GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(const KernTable& font,
	    zst::span<GlyphId> glyphs,
	    const FeatureSet& features);


	struct MorxFeature
	{
		aat::Feature feature;

		uint32_t enable_flags;
		uint32_t disable_flags;
	};

	struct MorxSubtableCommon
	{
		uint32_t flags;
		bool process_logical_order;
		bool process_descending_order;

		bool only_vertical;
		bool both_horizontal_and_vertical;

		zst::byte_span table_start;
	};

	struct MorxRearrangementSubtable
	{
		MorxSubtableCommon common;
		StateTable state_table;
	};

	struct MorxContextualSubtable
	{
		MorxSubtableCommon common;
		StateTable state_table;

		zst::byte_span substitution_tables;
	};

	struct MorxLigatureSubtable
	{
		MorxSubtableCommon common;
		StateTable state_table;

		util::big_endian_span<uint32_t> ligature_actions;
		util::big_endian_span<uint16_t> component_table;
		util::big_endian_span<uint16_t> ligatures;
	};

	struct MorxNonContextualSubtable
	{
		MorxSubtableCommon common;

		zst::byte_span lookup;
	};

	struct MorxInsertionSubtable
	{
		MorxSubtableCommon common;
		StateTable state_table;

		zst::byte_span insertion_glyph_table;
	};

	using MorxSubtable = std::variant<MorxRearrangementSubtable,
	    MorxContextualSubtable,
	    MorxLigatureSubtable,
	    MorxNonContextualSubtable,
	    MorxInsertionSubtable>;

	struct MorxChain
	{
		uint32_t default_flags;

		std::vector<MorxFeature> features;
		std::vector<MorxSubtable> subtables;
	};

	struct MorxTable
	{
		std::vector<MorxChain> chains;
		size_t num_font_glyphs;
	};

	std::optional<SubstitutedGlyphString> performSubstitutionsForGlyphSequence(const MorxTable& morx,
	    zst::span<GlyphId> glyphs,
	    const FeatureSet& features);
}

template <>
struct std::hash<font::aat::Feature>
{
	size_t operator()(font::aat::Feature f) const
	{
		return std::hash<uint32_t>()(((uint32_t) f.type << 16) | f.selector);
	}
};
