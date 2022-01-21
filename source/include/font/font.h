// font.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <string>
#include <vector>
#include <optional>

#include <cstddef>
#include <cstdint>

#include <zst.h>

#include "font/tag.h"
#include "font/features.h"

namespace pdf
{
	struct Stream;
}

namespace font
{
	namespace cff
	{
		struct CFFData;
	}

	namespace truetype
	{
		struct TTData;
	}

	struct FontMetrics
	{
		int xmin;
		int ymin;
		int xmax;
		int ymax;

		int ascent;
		int descent;
		int units_per_em;

		int x_height;
		int cap_height;

		bool is_monospaced;
		double italic_angle;
	};

	struct GlyphMetrics
	{
		double horz_advance;
		double vert_advance;

		double xmin;
		double xmax;
		double ymin;
		double ymax;

		double left_side_bearing;
		double right_side_bearing;
	};

	struct Table
	{
		Tag tag;
		uint32_t offset;
		uint32_t length;
		uint32_t checksum;
	};

	struct CMapTable
	{
		uint16_t platform_id;
		uint16_t encoding_id;
		uint32_t file_offset;
		uint32_t format;
	};

	using KerningPair = std::pair<GlyphAdjustment, GlyphAdjustment>;

	struct FontFile
	{
		static FontFile* parseFromFile(const std::string& path);

		uint32_t getGlyphIndexForCodepoint(uint32_t codepoint) const;
		GlyphMetrics getGlyphMetrics(uint32_t glyphId) const;

		std::optional<GlyphAdjustment> getGlyphAdjustment(uint32_t glyphId) const;
		std::optional<std::pair<GlyphAdjustment, GlyphAdjustment>> getGlyphPairAdjustments(uint32_t g1, uint32_t g2) const;

		std::map<uint32_t, GlyphLigatureSet> getAllGlyphLigatures() const;
		std::map<std::pair<uint32_t, uint32_t>, KerningPair> getAllKerningPairs() const;

		// corresponds to name IDs 16 and 17. if not present, they will have the same
		// value as their *_compat counterparts.
		std::string family;
		std::string subfamily;

		// corresponds to name IDs 1 and 2.
		std::string family_compat;
		std::string subfamily_compat;

		std::string unique_name;        // name 3

		std::string full_name;          // name 4
		std::string postscript_name;    // name 6

		// when embedding this font, we are probably legally required to reproduce
		// the license.
		std::string copyright_info;     // name 0
		std::string license_info;       // name 13

		// some stuff we need to save, internal use.
		size_t num_hmetrics = 0;
		zst::byte_span hmtx_table {};

		size_t num_glyphs = 0;

		off::GPosTable gpos_table {};
		off::GSubTable gsub_table {};


		int font_type = 0;
		int outline_type = 0;

		// note: this *MUST* be a std::map (ie. ordered) because the tables must be sorted by Tag.
		std::map<Tag, Table> tables {};

		// cache this so we don't look for it.
		CMapTable preferred_cmap {};

		FontMetrics metrics {};

		// only valid if outline_type == OUTLINES_TRUETYPE
		truetype::TTData* truetype_data = nullptr;

		// only valid if outline_type == OUTLINES_CFF
		cff::CFFData* cff_data = nullptr;


		uint8_t* file_bytes = nullptr;
		size_t file_size = 0;

		static constexpr int TYPE_OPEN_FONT    = 1;

		static constexpr int OUTLINES_TRUETYPE = 1;
		static constexpr int OUTLINES_CFF      = 2;
	};

	// the reason this takes in a `map<uint32_t, GlyphMetrics>` is that it's simply how the pdf::Font
	// knows about which glyphs are used.
	void writeFontSubset(FontFile* font, zst::str_view subset_name, pdf::Stream* stream,
		const std::map<uint32_t, GlyphMetrics>& used_glyphs);

	std::string generateSubsetName(FontFile* font);

	uint16_t peek_u16(const zst::byte_span& s);
	uint32_t peek_u32(const zst::byte_span& s);
	uint8_t consume_u8(zst::byte_span& s);
	uint16_t consume_u16(zst::byte_span& s);
	int16_t consume_i16(zst::byte_span& s);
	uint32_t consume_u24(zst::byte_span& s);
	uint32_t consume_u32(zst::byte_span& s);
}
