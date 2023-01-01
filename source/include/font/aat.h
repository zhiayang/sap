// aat.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

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
		std::vector<Pair> pairs;
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

	std::optional<SubstitutedGlyphString> performSubstitutionsForGlyphSequence(const KernTable& font, zst::span<GlyphId> glyphs);
}
