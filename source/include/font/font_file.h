// font_file.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"  // for hashmap
#include "types.h" // for GlyphId
#include "units.h" // for Scalar, operator+, Vector2<>::scalar_type

#include "font/tag.h" // for Tag
#include "font/aat.h"
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
		struct CFFSubset;
	}

	namespace truetype
	{
		struct TTData;
		struct TTSubset;
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
		~FontFile();

		const FontNames& names() const { return m_names; }
		const FontMetrics& metrics() const { return m_metrics; }
		const CharacterMapping& characterMapping() const { return m_character_mapping; }

		const std::map<Tag, Table>& sfntTables() const { return m_tables; }

		zst::byte_span bytes() const { return zst::byte_span(m_file_bytes, m_file_size); }

		bool hasCffOutlines() const { return m_outline_type == OUTLINES_CFF; }
		bool hasTrueTypeOutlines() const { return m_outline_type == OUTLINES_TRUETYPE; }

		size_t numGlyphs() const { return m_num_glyphs; }

		GlyphMetrics getGlyphMetrics(GlyphId glyphId) const;
		GlyphId getGlyphIndexForCodepoint(char32_t codepoint) const;

		FontVector2d getWordSize(zst::wstr_view word) const;
		std::vector<GlyphInfo> getGlyphInfosForString(zst::wstr_view text) const;

		void writeSubset(zst::str_view subset_name, pdf::Stream* stream);

		bool isGlyphUsed(GlyphId glyph_id) const { return m_used_glyphs.contains(glyph_id); }
		void markGlyphAsUsed(GlyphId glyph_id) const { m_used_glyphs.insert(glyph_id); }
		const auto& usedGlyphs() const { return m_used_glyphs; }

		std::map<size_t, GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(zst::span<GlyphId> glyphs,
		    const font::off::FeatureSet& features) const;

		std::optional<SubstitutedGlyphString> performSubstitutionsForGlyphSequence(zst::span<GlyphId> glyphs,
		    const font::off::FeatureSet& features) const;

		static std::optional<std::shared_ptr<FontFile>> fromHandle(FontHandle handle);

	private:
		FontFile(const uint8_t* bytes, size_t size);

		void parse_gpos_table(const Table& gpos);
		void parse_gsub_table(const Table& gsub);

		void parse_kern_table(const Table& kern);
		void parse_morx_table(const Table& morx);

		void parse_name_table(const Table& table);
		void parse_cmap_table(const Table& table);
		void parse_head_table(const Table& table);
		void parse_post_table(const Table& table);
		void parse_hhea_table(const Table& table);
		void parse_os2_table(const Table& table);
		void parse_hmtx_table(const Table& table);
		void parse_maxp_table(const Table& table);
		void parse_loca_table(const Table& table);
		void parse_glyf_table(const Table& table);
		void parse_cff_table(const Table& table);

		/*
		    Subset the CFF font (given in `file`), including only the used_glyphs. Returns a new CFF and cmap table
		    for embedding into the OTF font.
		*/
		cff::CFFSubset createCFFSubset(zst::str_view subset_name);
		truetype::TTSubset createTTSubset();

		static std::shared_ptr<FontFile> from_offset_table(zst::byte_span file_bytes, size_t start_of_offset_table);
		static std::optional<std::shared_ptr<FontFile>> from_postscript_name_in_collection(zst::byte_span ttc_file,
		    zst::str_view postscript_name);

		FontNames m_names;

		CharacterMapping m_character_mapping {};
		FontMetrics m_metrics {};

		// note: this *MUST* be a std::map (ie. ordered) because the tables must be sorted by Tag.
		std::map<Tag, Table> m_tables {};

		// some stuff we need to save, internal use.
		size_t m_num_hmetrics = 0;
		zst::byte_span m_hmtx_table {};

		size_t m_num_glyphs = 0;

		// only valid if outline_type == OUTLINES_CFF
		std::unique_ptr<cff::CFFData> m_cff_data {};

		// only valid if outline_type == OUTLINES_TRUETYPE
		std::unique_ptr<truetype::TTData> m_truetype_data {};

		// optional feature tables
		std::optional<off::GPosTable> m_gpos_table {};
		std::optional<off::GSubTable> m_gsub_table {};
		std::optional<aat::KernTable> m_kern_table {};
		std::optional<aat::MorxTable> m_morx_table {};

		// caches
		mutable std::unordered_set<GlyphId> m_used_glyphs {};
		mutable std::map<GlyphId, GlyphMetrics> m_glyph_metrics {};
		mutable util::hashmap<std::u32string, FontVector2d> m_word_size_cache {};
		mutable util::hashmap<std::u32string, std::vector<GlyphInfo>> m_glyph_infos_cache {};

		static constexpr int OUTLINES_TRUETYPE = 1;
		static constexpr int OUTLINES_CFF = 2;

		int m_outline_type = 0;

		const uint8_t* m_file_bytes = nullptr;
		size_t m_file_size = 0;
	};

	CharacterMapping readCMapTable(zst::byte_span table);
}
