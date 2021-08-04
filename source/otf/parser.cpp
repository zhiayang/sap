// parser.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <zpr.h>
#include <cstring>
#include <cassert>

#include "pool.h"
#include "util.h"
#include "error.h"
#include "otf/otf.h"

namespace otf
{
	// these are all BIG ENDIAN, because FUCK YOU.
	static uint16_t as_u16(const zst::byte_span& s)
	{
		assert(s.size() >= sizeof(2));
		return ((uint16_t) s[0] << 8)
			| ((uint16_t) s[1] << 0);
	}

	static uint32_t as_u24(const zst::byte_span& s)
	{
		assert(s.size() >= 3);
		return ((uint32_t) s[0] << 16)
			| ((uint32_t) s[1] << 8)
			| ((uint32_t) s[2] << 0);
	}

	static uint32_t as_u32(const zst::byte_span& s)
	{
		assert(s.size() >= 4);
		return ((uint32_t) s[0] << 24)
			| ((uint32_t) s[1] << 16)
			| ((uint32_t) s[2] << 8)
			| ((uint32_t) s[3] << 0);
	}

	static uint8_t consume_u8(zst::byte_span& s)
	{
		auto ret = s[0];
		s.remove_prefix(1);
		return ret;
	}

	static uint16_t consume_u16(zst::byte_span& s)
	{
		auto ret = as_u16(s);
		s.remove_prefix(2);
		return ret;
	}

	static int16_t consume_i16(zst::byte_span& s)
	{
		auto ret = as_u16(s);
		s.remove_prefix(2);
		return static_cast<int16_t>(ret);
	}

	static uint32_t consume_u24(zst::byte_span& s)
	{
		auto ret = as_u24(s);
		s.remove_prefix(3);
		return ret;
	}

	static uint32_t consume_u32(zst::byte_span& s)
	{
		auto ret = as_u32(s);
		s.remove_prefix(4);
		return ret;
	}

	static Table parse_table(zst::byte_span& buf)
	{
		if(buf.size() < 4 * sizeof(uint32_t))
			sap::internal_error("unexpected end of file (while parsing table header)");

		Table table { };
		table.tag = Tag(consume_u32(buf));
		table.checksum = consume_u32(buf);
		table.offset = consume_u32(buf);
		table.length = consume_u32(buf);

		zpr::println("found '{}' table, ofs={x}, length={x}", table.tag.str(), table.offset, table.length);
		return table;
	}

	static void parse_name_table(OTFont* font, const Table& name_table)
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

			if(nr.name_id == 0)         font->copyright_info = text;
			else if(nr.name_id == 1)    font->family_compat = text;
			else if(nr.name_id == 2)    font->subfamily_compat = text;
			else if(nr.name_id == 3)    font->unique_name = text;
			else if(nr.name_id == 4)    font->full_name = text;
			else if(nr.name_id == 6)    font->postscript_name = text;
			else if(nr.name_id == 13)   font->license_info = text;
			else if(nr.name_id == 16)   font->family = text;
			else if(nr.name_id == 17)   font->subfamily = text;
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

	static void parse_cmap_subtable_0(OTFont* font, zst::byte_span subtable)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 0);

		auto len = consume_u16(subtable);
		assert(len == 3 * sizeof(uint16_t) + 256 * sizeof(uint8_t));

		auto lang = consume_u16(subtable);
		(void) lang;

		for(size_t i = 0; i < 255; i++)
			font->cmap[i] = consume_u8(subtable);
	}

	static void parse_cmap_subtable_4(OTFont* font, zst::byte_span subtable)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 4);

		// assert(!"TODO");
	}

	static void parse_cmap_subtable_6(OTFont* font, zst::byte_span subtable)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 6);

		auto len = consume_u16(subtable);
		auto lang = consume_u16(subtable);

		(void) len;
		(void) lang;

		auto first = consume_u16(subtable);
		auto count = consume_u16(subtable);

		for(size_t i = 0; i < count; i++)
			font->cmap[i + first] = consume_u16(subtable);
	}

	static void parse_cmap_subtable_10(OTFont* font, zst::byte_span subtable)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 10);

		consume_u16(subtable);  // reserved
		auto len = consume_u32(subtable);
		auto lang = consume_u32(subtable);

		(void) len;
		(void) lang;

		auto first = consume_u32(subtable);
		auto count = consume_u32(subtable);

		for(size_t i = 0; i < count; i++)
			font->cmap[i + first] = consume_u32(subtable);
	}


	static void parse_cmap_subtable_12_or_13(OTFont* font, zst::byte_span subtable)
	{
		auto fmt = consume_u16(subtable);
		assert(fmt == 12 || fmt == 13);

		consume_u16(subtable);  // reserved
		auto len = consume_u32(subtable);
		auto lang = consume_u32(subtable);

		(void) len;
		(void) lang;

		auto num_groups = consume_u32(subtable);
		for(size_t i = 0; i < num_groups; i++)
		{
			auto start = consume_u32(subtable);
			auto end = consume_u32(subtable);
			auto gid = consume_u32(subtable);

			for(auto i = start; i <= end; i++)
			{
				if(fmt == 12)   font->cmap[i] = gid + i - start;
				else            font->cmap[i] = gid;
			}
		}
	}

	static void parse_cmap_table(OTFont* font, const Table& cmap_table)
	{
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(cmap_table.offset);

		const auto table_start = buf;

		auto version = consume_u16(buf);
		(void) version;

		auto num_tables = consume_u16(buf);
		zpr::println("{} encoding records", num_tables);

		struct subtable_t
		{
			int platform_id;
			int encoding_id;
			uint32_t offset;
			int format;
		};

		std::vector<subtable_t> subtables;


		for(size_t i = 0; i < num_tables; i++)
		{
			auto platform_id = consume_u16(buf);
			auto encoding_id = consume_u16(buf);
			auto offset = consume_u32(buf);

			zpr::println("  pid = {}, eid = {}", platform_id, encoding_id);

			auto subtable = table_start.drop(offset);
			auto format = consume_u16(subtable);
			zpr::println("  format = {}", format);

			subtables.push_back({ platform_id, encoding_id, offset, format });
		}


		/*
			preferred table order:

			1. unicode (0): 6, 4, 3
			2. windows (3): 10, 1
			3. macos (1): 0
		*/

		bool found = false;
		subtable_t chosen_table { };
		int asdf[][2] = { { 0, 6 }, { 0, 4 }, { 0, 3 }, { 3, 10 }, { 3, 1 }, { 1, 0 } };

		for(auto& [ p, e ] : asdf)
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
			sap::internal_error("could not find suitable cmap table");

		zpr::println("found cmap: pid {}, eid {}", chosen_table.platform_id, chosen_table.encoding_id);

		auto subtable_start = table_start.drop(chosen_table.offset);
		if(chosen_table.format == 0)        parse_cmap_subtable_0(font, subtable_start);
		else if(chosen_table.format == 4)   parse_cmap_subtable_4(font, subtable_start);
		else if(chosen_table.format == 6)   parse_cmap_subtable_6(font, subtable_start);
		else if(chosen_table.format == 10)  parse_cmap_subtable_10(font, subtable_start);
		else if(chosen_table.format == 12)  parse_cmap_subtable_12_or_13(font, subtable_start);
		else if(chosen_table.format == 13)  parse_cmap_subtable_12_or_13(font, subtable_start);
		else sap::internal_error("unsupported subtable format {}", chosen_table.format);
	}




	static void parse_head_table(OTFont* font, const Table& head_table)
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
	}

	static void parse_post_table(OTFont* font, const Table& post_table)
	{
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(post_table.offset);

		// for now we only need the italic angle and the "isfixedpitch" flag.
		auto version = consume_u32(buf); (void) version;

		{
			auto whole = consume_i16(buf);
			auto frac = consume_u16(buf);

			font->metrics.italic_angle = static_cast<double>(whole) + (static_cast<double>(frac) / 65536.0);
		}


		consume_u32(buf); // skip the two underline-related metrics

		font->metrics.is_monospaced = consume_u32(buf) != 0;
	}

	static void parse_hhea_table(OTFont* font, const Table& hhea_table)
	{
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(hhea_table.offset);

		// skip the versions
		buf.remove_prefix(4);

		// note: this is the raw value; pdf wants some scaled value that needs
		// units_per_em to compute (specifically, (ascent / units_per_em) * 1000), but
		// that princess is in another castle and it's too much hassle here, so leave it
		// to the pdf layer to convert.
		font->metrics.ascent = static_cast<int16_t>(consume_i16(buf));
		font->metrics.descent = static_cast<int16_t>(consume_i16(buf));
	}

	static void parse_os2_table(OTFont* font, const Table& os2_table)
	{
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(os2_table.offset);

		// we only want this table for sxHeight and sCapHeight, but they only appear
		// in version >= 2. so if it's less then just gtfo.
		auto version = consume_u16(buf);
		if(version < 2) return;

		buf.remove_prefix(84);
		font->metrics.x_height = consume_i16(buf);
		font->metrics.cap_height = consume_i16(buf);
	}


















	OTFont* OTFont::parseFromFile(const std::string& path)
	{
		auto font = util::make<OTFont>();
		std::tie(font->file_bytes, font->file_size) = util::readEntireFile(path);

		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		if(buf.size() < 32)
			sap::internal_error("file too short (< 32 bytes)");

		// first get the version.
		auto sfnt_version = Tag(consume_u32(buf));

		if(sfnt_version == Tag("OTTO"))
			font->font_type = TYPE_CFF;

		else if(sfnt_version == Tag(0, 1, 0, 0))
			font->font_type = TYPE_TRUETYPE;

		else
			sap::internal_error("unsupported ttf/otf file; unknown header bytes '{}'", sfnt_version.str());

		auto num_tables = consume_u16(buf);
		auto search_range = consume_u16(buf);
		auto entry_selector = consume_u16(buf);
		auto range_shift = consume_u16(buf);

		(void) search_range;
		(void) entry_selector;
		(void) range_shift;

		for(size_t i = 0; i < num_tables; i++)
		{
			auto tbl = parse_table(buf);
			if(tbl.tag == Tag("name"))      parse_name_table(font, tbl);
			else if(tbl.tag == Tag("cmap")) parse_cmap_table(font, tbl);
			else if(tbl.tag == Tag("head")) parse_head_table(font, tbl);
			else if(tbl.tag == Tag("post")) parse_post_table(font, tbl);
			else if(tbl.tag == Tag("hhea")) parse_hhea_table(font, tbl);
			else if(tbl.tag == Tag("OS/2")) parse_os2_table(font, tbl);

			font->tables.emplace(tbl.tag, tbl);
		}

		return font;
	}
}
