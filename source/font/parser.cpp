// parser.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <zpr.h>
#include <cstring>
#include <cassert>

#include "pool.h"
#include "util.h"
#include "error.h"

#include "font/cff.h"
#include "font/font.h"
#include "font/truetype.h"

namespace font
{
	// these are all BIG ENDIAN, because FUCK YOU.
	uint16_t peek_u16(const zst::byte_span& s)
	{
		assert(s.size() >= 2);
		return ((uint16_t) s[0] << 8) | ((uint16_t) s[1] << 0);
	}

	int16_t peek_i16(const zst::byte_span& s)
	{
		return static_cast<int16_t>(peek_u16(s));
	}

	uint32_t peek_u32(const zst::byte_span& s)
	{
		assert(s.size() >= 4);
		return ((uint32_t) s[0] << 24) | ((uint32_t) s[1] << 16) | ((uint32_t) s[2] << 8) | ((uint32_t) s[3] << 0);
	}

	uint8_t consume_u8(zst::byte_span& s)
	{
		auto ret = s[0];
		s.remove_prefix(1);
		return ret;
	}

	uint16_t consume_u16(zst::byte_span& s)
	{
		auto ret = peek_u16(s);
		s.remove_prefix(2);
		return ret;
	}

	int16_t consume_i16(zst::byte_span& s)
	{
		auto ret = peek_u16(s);
		s.remove_prefix(2);
		return static_cast<int16_t>(ret);
	}

	uint32_t consume_u32(zst::byte_span& s)
	{
		auto ret = peek_u32(s);
		s.remove_prefix(4);
		return ret;
	}

	static void parse_name_table(FontFile* font, const Table& name_table)
	{
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(name_table.offset);

		// make a copy since we'll be consuming buf
		auto storage = buf;

		auto format = consume_u16(buf);
		auto num = consume_u16(buf);
		auto ofs = consume_u16(buf);

		// now storage points at the start of the variable storage space.
		storage.remove_prefix(ofs);

		struct NameRecord
		{
			uint16_t platform_id;
			uint16_t encoding_id;
			uint16_t language_id;
			uint16_t name_id;
			uint16_t length;
			uint16_t offset;
		};

		struct LangTagRecord
		{
			uint16_t length;
			uint16_t offset;
		};

		std::vector<NameRecord> name_records;
		std::vector<LangTagRecord> lang_records;

		// always parse the name record
		for(size_t i = 0; i < num; i++)
		{
			// i hate this.
			auto a = consume_u16(buf);
			auto b = consume_u16(buf);
			auto c = consume_u16(buf);
			auto d = consume_u16(buf);
			auto e = consume_u16(buf);
			auto f = consume_u16(buf);

			name_records.push_back({ a, b, c, d, e, f });
		}

		if(format == 1)
		{
			auto tmp = consume_u16(buf);
			for(size_t i = 0; i < tmp; i++)
			{
				auto a = consume_u16(buf);
				auto b = consume_u16(buf);
				lang_records.push_back({ a, b });
			}
		}

		// TODO: non-english things are not even handled rn.
		for(auto& nr : name_records)
		{
			// TODO: for now, we don't support any encoding other than Windows + Unicode BMP.
			// (which includes surrogate pairs, i think -- since the OTF spec explicitly
			// mentions "UTF-16BE")

			// I **think** most fonts will have at least the standard names in plat=3 + enc=1;
			// some ttf files have two copies -- one MacRoman (plat=1, enc=0) and one
			// windows utf16 (plat=3, enc=1) -- so we **should** be fine for now.
			if(nr.platform_id == 3 && nr.encoding_id != 1)
				continue;

			auto u16 = storage.drop(nr.offset).take(nr.length);
			auto text = unicode::utf8FromUtf16BigEndianBytes(u16);

			if(nr.name_id == 0)
				font->copyright_info = text;
			else if(nr.name_id == 1)
				font->family_compat = text;
			else if(nr.name_id == 2)
				font->subfamily_compat = text;
			else if(nr.name_id == 3)
				font->unique_name = text;
			else if(nr.name_id == 4)
				font->full_name = text;
			else if(nr.name_id == 6)
				font->postscript_name = text;
			else if(nr.name_id == 13)
				font->license_info = text;
			else if(nr.name_id == 16)
				font->family = text;
			else if(nr.name_id == 17)
				font->subfamily = text;
		}

		if(font->family.empty())
			font->family = font->family_compat;

		if(font->subfamily.empty())
			font->subfamily = font->subfamily_compat;

		if(font->postscript_name.empty())
		{
			// TODO: need to remove spaces here
			zpr::fprintln(stderr, "warning: making up a postscript name, using '{}'", font->unique_name);
			font->postscript_name = font->unique_name;
		}
	}

	static void parse_cmap_table(FontFile* font, const Table& cmap_table)
	{
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(cmap_table.offset);

		const auto table_start = buf;

		auto version = consume_u16(buf);
		(void) version;

		auto num_tables = consume_u16(buf);
		// zpr::println("{} encoding records", num_tables);

		struct CMapTable
		{
			uint16_t platform_id;
			uint16_t encoding_id;
			uint32_t offset;
		};

		std::vector<CMapTable> subtables {};
		for(size_t i = 0; i < num_tables; i++)
		{
			auto platform_id = consume_u16(buf);
			auto encoding_id = consume_u16(buf);
			auto offset = consume_u32(buf);

			// zpr::println("  pid = {}, eid = {}", platform_id, encoding_id);
			subtables.push_back(CMapTable { platform_id, encoding_id, offset });
		}

		/*
		    preferred table order:

		    1. unicode (0): 6, 4, 3
		    2. windows (3): 10, 1
		    3. macos (1): 0

		    TODO: we might also want to discriminate based on the format kind?
		*/

		bool found = false;
		CMapTable chosen_table {};
		int asdf[][2] = { { 0, 6 }, { 0, 4 }, { 0, 3 }, { 3, 10 }, { 3, 1 }, { 1, 0 } };

		for(auto& [p, e] : asdf)
		{
			for(auto& tbl : subtables)
			{
				if(tbl.platform_id == p && tbl.encoding_id == e)
				{
					chosen_table = tbl;
					found = true;
					break;
				}
			}

			if(found)
				break;
		}

		if(!found)
			sap::error("font/off", "could not find suitable cmap table");

		auto foo = table_start.drop(chosen_table.offset);
		font->character_mapping = readCMapTable(foo);

		// zpr::println("found cmap: pid {}, eid {}, format {}", chosen_table.platform_id,
		//	chosen_table.encoding_id, chosen_table.format);
	}




	static void parse_head_table(FontFile* font, const Table& head_table)
	{
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(head_table.offset);

		// skip the following: major ver (16), minor ver (16), font rev (32), checksum (32), magicnumber (32),
		// flags (16), unitsperem (16), created (64), modified (64).
		buf.remove_prefix(18);

		font->metrics.units_per_em = consume_u16(buf);

		buf.remove_prefix(16);

		font->metrics.xmin = consume_i16(buf);
		font->metrics.ymin = consume_i16(buf);
		font->metrics.xmax = consume_i16(buf);
		font->metrics.ymax = consume_i16(buf);

		// skip some stuff
		buf.remove_prefix(6);

		// by the time we reach this place, the tt_data should have been created
		// by the `glyf` table already.
		auto loca_bytes = (consume_u16(buf) == 0) ? 2 : 4;
		if(font->outline_type == FontFile::OUTLINES_TRUETYPE)
		{
			assert(font->truetype_data != nullptr);
			font->truetype_data->loca_bytes_per_entry = loca_bytes;
		}
	}

	static void parse_post_table(FontFile* font, const Table& post_table)
	{
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(post_table.offset);

		// for now we only need the italic angle and the "isfixedpitch" flag.
		auto version = consume_u32(buf);
		(void) version;

		{
			auto whole = consume_i16(buf);
			auto frac = consume_u16(buf);

			font->metrics.italic_angle = static_cast<double>(whole) + (static_cast<double>(frac) / 65536.0);
		}


		consume_u32(buf); // skip the two underline-related metrics

		font->metrics.is_monospaced = consume_u32(buf) != 0;
	}

	static void parse_hhea_table(FontFile* font, const Table& hhea_table)
	{
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(hhea_table.offset);

		// skip the versions
		buf.remove_prefix(4);

		// we should have already parsed the head table, so this value should be present.
		assert(font->metrics.units_per_em != 0);

		font->metrics.hhea_ascent = consume_i16(buf);
		font->metrics.hhea_descent = consume_i16(buf);
		font->metrics.hhea_linegap = consume_i16(buf);

		// skip all the nonsense
		buf.remove_prefix(24);

		font->num_hmetrics = consume_u16(buf);
	}

	static void parse_os2_table(FontFile* font, const Table& os2_table)
	{
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(os2_table.offset);

		auto version = consume_u16(buf);

		// the sTypo things are always available, even at version 0
		font->metrics.typo_ascent = peek_i16(buf.drop(66));
		font->metrics.typo_descent = peek_i16(buf.drop(68));
		font->metrics.typo_linegap = peek_i16(buf.drop(70));

		// "it is strongly recommended to use OS/2.sTypoAscender - OS/2.sTypoDescender+ OS/2.sTypoLineGap
		// as a value for default line spacing for this font."
		// (note: we do this regardless of whether fsSelection & USE_TYPO_METRICS is true)
		// TODO: check if these values are 0 or something -- though they shouldn't be
		font->metrics.default_line_spacing = font->metrics.typo_ascent - font->metrics.typo_descent + font->metrics.typo_linegap;

		if(version >= 2)
		{
			font->metrics.x_height = peek_i16(buf.drop(84));
			font->metrics.cap_height = peek_i16(buf.drop(84 + 2));
		}
	}

	static void parse_hmtx_table(FontFile* font, const Table& hmtx_table)
	{
		// hhea should have been parsed already, so this should be present.
		assert(font->num_hmetrics > 0);
		font->hmtx_table = zst::byte_span(font->file_bytes, font->file_size).drop(hmtx_table.offset).take(hmtx_table.length);

		// deferred processing -- we only take the glyph data when it is requested.
	}

	static void parse_maxp_table(FontFile* font, const Table& maxp_table)
	{
		// we are only interested in the number of glyphs, so the version is
		// not really relevant (neither is any other field)
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(maxp_table.offset);

		auto version = consume_u32(buf);
		(void) version;

		font->num_glyphs = consume_u16(buf);
	}


	static void parse_loca_table(FontFile* font, const Table& loca_table)
	{
		if(font->outline_type != FontFile::OUTLINES_TRUETYPE)
			sap::internal_error("found 'loca' table in file with non-truetype outlines");

		assert(font->truetype_data != nullptr);
		auto table = zst::byte_span(font->file_bytes, font->file_size).drop(loca_table.offset).take(loca_table.length);

		truetype::parseLocaTable(font, table);
	}

	static void parse_glyf_table(FontFile* font, const Table& glyf_table)
	{
		if(font->outline_type != FontFile::OUTLINES_TRUETYPE)
			sap::internal_error("found 'glyf' table in file with non-truetype outlines");

		assert(font->truetype_data != nullptr);
		auto table = zst::byte_span(font->file_bytes, font->file_size).drop(glyf_table.offset).take(glyf_table.length);

		truetype::parseGlyfTable(font, table);
	}

	static void parse_cff_table(FontFile* font, const Table& cff_table)
	{
		if(font->outline_type != FontFile::OUTLINES_CFF)
			sap::internal_error("found 'CFF' table in file with non-CFF outlines");

		auto cff_data = zst::byte_span(font->file_bytes, font->file_size).drop(cff_table.offset).take(cff_table.length);

		font->cff_data = cff::parseCFFData(font, cff_data);
	}









	/*
	    TODO: we need a better way to parse this. probably some kind of Parser
	    struct that allows marking offset starts and other stuff.
	*/
	static Table parse_table(zst::byte_span& buf)
	{
		if(buf.size() < 4 * sizeof(uint32_t))
			sap::internal_error("unexpected end of file (while parsing table header)");

		Table table {};
		table.tag = Tag(consume_u32(buf));
		table.checksum = consume_u32(buf);
		table.offset = consume_u32(buf);
		table.length = consume_u32(buf);

		// zpr::println("    found '{}' table, ofs={x}, length={x}", table.tag.str(), table.offset, table.length);
		return table;
	}

	static FontFile* parseOTF(zst::byte_span buf)
	{
		// this is perfectly fine, because we own the data referred to by 'buf'.
		auto font = util::make<FontFile>();
		font->file_bytes = const_cast<uint8_t*>(buf.data());
		font->file_size = buf.size();

		// first get the version.
		auto sfnt_version = Tag(consume_u32(buf));

		if(sfnt_version == Tag("OTTO"))
			font->outline_type = FontFile::OUTLINES_CFF;

		else if(sfnt_version == Tag(0, 1, 0, 0) || sfnt_version == Tag("true"))
			font->outline_type = FontFile::OUTLINES_TRUETYPE;

		else
			sap::internal_error("unsupported ttf/otf file; unknown header bytes '{}'", sfnt_version.str());


		auto num_tables = consume_u16(buf);
		auto search_range = consume_u16(buf);
		auto entry_selector = consume_u16(buf);
		auto range_shift = consume_u16(buf);

		// zpr::println("{} tables", num_tables);

		(void) search_range;
		(void) entry_selector;
		(void) range_shift;

		std::map<Tag, Table> parsed_tables {};
		for(size_t i = 0; i < num_tables; i++)
		{
			auto tbl = parse_table(buf);
			parsed_tables[tbl.tag] = std::move(tbl);
			font->tables.emplace(tbl.tag, tbl);
		}

		// there is an order that we want to use:
		constexpr Tag table_processing_order[] = { Tag("head"), Tag("name"), Tag("hhea"), Tag("hmtx"), Tag("maxp"), Tag("post"),
			Tag("CFF "), Tag("CFF2"), Tag("glyf"), Tag("loca"), Tag("cmap"), Tag("GPOS"), Tag("GSUB"), Tag("OS/2") };

		// CFF makes its own data (since everything is self-contained in the CFF table)
		// but for TrueType, it's split across several tables, so just make one here.
		if(font->outline_type == FontFile::OUTLINES_TRUETYPE)
			font->truetype_data = util::make<truetype::TTData>();

		for(auto tag : table_processing_order)
		{
			if(auto it = parsed_tables.find(tag); it != parsed_tables.end())
			{
				auto& tbl = it->second;
				if(tag == Tag("CFF "))
					parse_cff_table(font, tbl);
				else if(tag == Tag("CFF2"))
					parse_cff_table(font, tbl);
				else if(tag == Tag("GPOS"))
					off::parseGPos(font, tbl);
				else if(tag == Tag("GSUB"))
					off::parseGSub(font, tbl);
				else if(tag == Tag("OS/2"))
					parse_os2_table(font, tbl);
				else if(tag == Tag("cmap"))
					parse_cmap_table(font, tbl);
				else if(tag == Tag("glyf"))
					parse_glyf_table(font, tbl);
				else if(tag == Tag("head"))
					parse_head_table(font, tbl);
				else if(tag == Tag("hhea"))
					parse_hhea_table(font, tbl);
				else if(tag == Tag("hmtx"))
					parse_hmtx_table(font, tbl);
				else if(tag == Tag("loca"))
					parse_loca_table(font, tbl);
				else if(tag == Tag("maxp"))
					parse_maxp_table(font, tbl);
				else if(tag == Tag("name"))
					parse_name_table(font, tbl);
				else if(tag == Tag("post"))
					parse_post_table(font, tbl);
			}
		}
		return font;
	}




	FontFile* FontFile::parseFromFile(const std::string& path)
	{
		// zpr::println("read {}", path);
		auto [buf, len] = util::readEntireFile(path);
		if(len < 4)
			sap::internal_error("font file too short");

		if(memcmp(buf, "OTTO", 4) == 0 || memcmp(buf, "true", 4) == 0 || memcmp(buf, "\x00\x01\x00\x00", 4) == 0)
			return parseOTF(zst::byte_span(buf, len));

		else
			sap::internal_error("unsupported font file; unknown header bytes '{}'", zst::str_view((char*) buf, 4));
	}
}
