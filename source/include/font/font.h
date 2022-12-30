// font.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"  // for hashmap
#include "types.h" // for GlyphId
#include "units.h" // for Scalar, operator+, Vector2<>::scalar_type

#include "font/tag.h"         // for Tag
#include "font/handle.h"      // for FontHandle
#include "font/features.h"    // for GlyphAdjustment, Feature, LookupTable
#include "font/font_scalar.h" // for FontScalar, FontVector2d, font_design_space

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

		int hhea_ascent;
		int hhea_descent;
		int hhea_linegap;
		int units_per_em;

		int x_height;
		int cap_height;

		int typo_ascent;
		int typo_descent;
		int typo_linegap;

		FontScalar default_line_spacing;

		bool is_monospaced;
		double italic_angle;
	};

	struct GlyphMetrics
	{
		FontScalar horz_advance;
		FontScalar vert_advance;

		FontScalar horz_placement;
		FontScalar vert_placement;

		FontScalar xmin;
		FontScalar xmax;
		FontScalar ymin;
		FontScalar ymax;

		FontScalar left_side_bearing;
		FontScalar right_side_bearing;
	};

	struct GlyphInfo
	{
		GlyphId gid;
		GlyphMetrics metrics;
		GlyphAdjustment adjustments;
	};

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

	using KerningPair = std::pair<GlyphAdjustment, GlyphAdjustment>;


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

		std::vector<GlyphInfo> getGlyphInfosForString(zst::wstr_view text)
		{
			// first, convert all codepoints to glyphs
			std::vector<GlyphId> glyphs {};
			for(char32_t cp : text)
				glyphs.push_back(this->getGlyphIndexForCodepoint(cp));

			using font::Tag;
			font::off::FeatureSet features {};

			// REMOVE: this is just for testing!
			features.script = Tag("cyrl");
			features.language = Tag("BGR ");
			features.enabled_features = { Tag("kern"), Tag("liga"), Tag("locl") };

			auto span = [](auto& foo) {
				return zst::span<GlyphId>(foo.data(), foo.size());
			};

			// next, use GSUB to perform substitutions.
			// ignore glyph mappings because we only care about the final result
			glyphs = font::off::performSubstitutionsForGlyphSequence(this, span(glyphs), features).glyphs;

			// next, get base metrics for each glyph.
			std::vector<GlyphInfo> glyph_infos {};
			for(auto g : glyphs)
			{
				GlyphInfo info {};
				info.gid = g;
				info.metrics = this->getGlyphMetrics(g);
				glyph_infos.push_back(std::move(info));
			}

			// finally, use GPOS
			auto adjustment_map = font::off::getPositioningAdjustmentsForGlyphSequence(this, span(glyphs), features);
			for(auto& [i, adj] : adjustment_map)
			{
				auto& info = glyph_infos[i];
				info.adjustments.horz_advance += adj.horz_advance;
				info.adjustments.vert_advance += adj.vert_advance;
				info.adjustments.horz_placement += adj.horz_placement;
				info.adjustments.vert_placement += adj.vert_placement;
			}

			return glyph_infos;
		}

		static inline util::hashmap<std::u32string, FontVector2d> s_word_size_cache {};
		FontVector2d get_word_size(zst::wstr_view string)
		{
			if(auto it = s_word_size_cache.find(string); it != s_word_size_cache.end())
				return it->second;

			FontVector2d size;
			size.y() = this->metrics.default_line_spacing;
			auto glyphs = this->getGlyphInfosForString(string);
			for(auto& g : glyphs)
			{
				size.x() += g.metrics.horz_advance + g.adjustments.horz_advance;
			}

			s_word_size_cache[string.str()] = size;
			return size;
		}

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

	std::string generateSubsetName(FontFile* font);

	uint16_t peek_u16(const zst::byte_span& s);
	uint32_t peek_u32(const zst::byte_span& s);
	uint8_t consume_u8(zst::byte_span& s);
	uint16_t consume_u16(zst::byte_span& s);
	int16_t consume_i16(zst::byte_span& s);
	uint32_t consume_u24(zst::byte_span& s);
	uint32_t consume_u32(zst::byte_span& s);
}
