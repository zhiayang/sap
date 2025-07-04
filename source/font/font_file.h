// font_file.h
// Copyright (c) 2021, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"
#include "types.h"
#include "units.h"

#include "font/aat.h"
#include "font/off.h"
#include "font/tag.h"
#include "font/handle.h"
#include "font/metrics.h"
#include "font/features.h"
#include "font/font_source.h"
#include "font/font_scalar.h"

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

		// other names
		util::hashmap<uint16_t, std::string> others;
	};

	struct FontFile : FontSource
	{
		// declare this out of line so we can have std::unique_ptr to incomplete types
		~FontFile();

		const FontNames& names() const { return m_names; }

		const std::map<Tag, Table>& sfntTables() const { return m_tables; }

		zst::byte_span bytes() const { return zst::byte_span(m_file.get(), m_file.size()); }

		bool hasCffOutlines() const { return m_outline_type == OUTLINES_CFF; }
		bool hasTrueTypeOutlines() const { return m_outline_type == OUTLINES_TRUETYPE; }

		void writeSubset(zst::str_view subset_name, pdf::Stream* stream);

		virtual bool isBuiltin() const override { return false; }
		virtual std::string name() const override { return m_names.postscript_name; }

		virtual util::hashmap<size_t, GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(zst::span<GlyphId>
		                                                                                             glyphs,
		    const font::FeatureSet& features) const override;

		virtual std::optional<SubstitutedGlyphString> performSubstitutionsForGlyphSequence(zst::span<GlyphId> glyphs,
		    const font::FeatureSet& features) const override;

		static std::optional<std::unique_ptr<FontFile>> fromHandle(FontHandle handle);

	private:
		FontFile(zst::unique_span<uint8_t[]> bytes);

		friend struct FontSource;
		virtual GlyphMetrics get_glyph_metrics_impl(GlyphId glyphId) const override;

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

		static std::unique_ptr<FontFile> from_offset_table(zst::unique_span<uint8_t[]> bytes, //
		    size_t start_of_offset_table);
		static std::optional<std::unique_ptr<FontFile>> from_postscript_name_in_collection(zst::unique_span<uint8_t[]>
		                                                                                       bytes,
		    zst::str_view postscript_name);

		FontNames m_names;

		// note: this *MUST* be a std::map (ie. ordered) because the tables must be sorted by Tag.
		std::map<Tag, Table> m_tables {};

		// some stuff we need to save, internal use.
		size_t m_num_hmetrics = 0;
		zst::byte_span m_hmtx_table {};

		// only valid if outline_type == OUTLINES_CFF
		std::unique_ptr<cff::CFFData> m_cff_data {};

		// only valid if outline_type == OUTLINES_TRUETYPE
		std::unique_ptr<truetype::TTData> m_truetype_data {};

		// optional feature tables
		std::optional<off::GPosTable> m_gpos_table {};
		std::optional<off::GSubTable> m_gsub_table {};
		std::optional<aat::KernTable> m_kern_table {};
		std::optional<aat::MorxTable> m_morx_table {};

		static constexpr int OUTLINES_TRUETYPE = 1;
		static constexpr int OUTLINES_CFF = 2;

		int m_outline_type = 0;

		zst::unique_span<uint8_t[]> m_file;
	};

	CharacterMapping readCMapTable(zst::byte_span table);
}
