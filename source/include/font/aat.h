// aat.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <variant>

#include "types.h" // for GlyphId

#include "font/tag.h"         // for Tag
#include "font/metrics.h"     //
#include "font/font_scalar.h" // for FontScalar

namespace font
{
	struct Table;
	struct FontFile;
}

// stuff dealing with Apple Advanced Typography tables
// (basically apple's own version of GPOS, GSUB, GDEF, etc.)

namespace font::aat
{
	/*
	    lookup tables
	*/
	struct Lookup
	{
		std::unordered_map<GlyphId, uint64_t> map;
	};

	std::optional<Lookup> parseLookupTable(zst::byte_span buf, size_t num_font_glyphs);


	/*
	    encodes a state machine
	*/
	struct StateTable
	{
		size_t num_classes;
		std::unordered_map<GlyphId, uint16_t> glyph_classes;

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

		std::unordered_map<GlyphId, uint16_t> left_glyph_classes;
		std::unordered_map<GlyphId, uint16_t> right_glyph_classes;

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

	std::map<size_t, GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(const KernTable& font, zst::span<GlyphId> glyphs);


	struct MorxFeature
	{
		uint16_t type;
		uint16_t selector;

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

		zst::byte_span ligature_actions;
		zst::byte_span component_table;
		zst::byte_span ligatures;
	};

	struct MorxNonContextualSubtable
	{
		MorxSubtableCommon common;
		Lookup lookup;
	};

	struct MorxInsertionSubtable
	{
		MorxSubtableCommon common;
		StateTable state_table;

		zst::byte_span insertion_glyph_table;
	};

	using MorxSubtable = std::variant<MorxRearrangementSubtable, MorxContextualSubtable, MorxLigatureSubtable,
	    MorxNonContextualSubtable, MorxInsertionSubtable>;

	struct MorxChain
	{
		uint32_t default_flags;

		std::vector<MorxFeature> features;
		std::vector<MorxSubtable> subtables;
	};

	struct MorxTable
	{
		std::vector<MorxChain> chains;
	};

	std::optional<SubstitutedGlyphString> performSubstitutionsForGlyphSequence(const MorxTable& morx, zst::span<GlyphId> glyphs);
}
