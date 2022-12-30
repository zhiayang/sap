// font_file.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"  // for hashmap
#include "types.h" // for GlyphId
#include "units.h" // for Scalar, operator+, Vector2<>::scalar_type

#include "font/tag.h"         // for Tag
#include "font/handle.h"      // for FontHandle
#include "font/metrics.h"     //
#include "font/features.h"    // for GlyphAdjustment, Feature, LookupTable
#include "font/font_scalar.h" // for FontScalar, FontVector2d, font_design_...

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

	struct Table
	{
		Tag tag;
		uint32_t offset;
		uint32_t length;
		uint32_t checksum;
	};

	struct CharacterMapping
	{
		std::unordered_map<char32_t, GlyphId> forward;
		std::unordered_map<GlyphId, char32_t> reverse;
	};

	struct FontNames
	{
		// corresponds to name IDs 16 and 17. if not present, they will have the same
		// value as their *_compat counterparts.
		std::string family;
		std::string subfamily;

		// corresponds to name IDs 1 and 2.
		std::string family_compat;
		std::string subfamily_compat;

		std::string unique_name; // name 3

		std::string full_name;       // name 4
		std::string postscript_name; // name 6

		// when embedding this font, we are probably legally required to reproduce the license.
		std::string copyright_info; // name 0
		std::string license_info;   // name 13
	};

	// TODO: clean up this entire struct
	struct FontFile
	{
		static FontFile* parseFromFile(const std::string& path);

		static std::optional<FontFile*> fromHandle(FontHandle handle);

		const FontNames& names() const { return m_names; }

		GlyphMetrics getGlyphMetrics(GlyphId glyphId) const;
		GlyphId getGlyphIndexForCodepoint(char32_t codepoint) const;
		std::vector<GlyphInfo> getGlyphInfosForString(zst::wstr_view text) const;

		FontNames m_names;

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

		CharacterMapping character_mapping {};

		FontMetrics metrics {};

		// only valid if outline_type == OUTLINES_TRUETYPE
		truetype::TTData* truetype_data = nullptr;

		// only valid if outline_type == OUTLINES_CFF
		cff::CFFData* cff_data = nullptr;


		uint8_t* file_bytes = nullptr;
		size_t file_size = 0;

		static constexpr int TYPE_OPEN_FONT = 1;

		static constexpr int OUTLINES_TRUETYPE = 1;
		static constexpr int OUTLINES_CFF = 2;
	};

	void writeFontSubset(FontFile* font, zst::str_view subset_name, pdf::Stream* stream,
	    const std::unordered_set<GlyphId>& used_glyphs);

	CharacterMapping readCMapTable(zst::byte_span table);
}
