// loader.cpp
// Copyright (c) 2021, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for readEntireFile, utf8FromUtf16BigEndian...

#include "font/cff.h"         // for parseCFFData
#include "font/tag.h"         // for Tag
#include "font/handle.h"      // for FontHandle
#include "font/features.h"    // for parseGPos, parseGSub
#include "font/truetype.h"    // for TTData, parseGlyfTable, parseLocaTable
#include "font/font_file.h"   // for FontFile, Table, FontNames, FontMetrics
#include "font/font_scalar.h" // for FontScalar

namespace misc
{
	extern std::string convert_mac_roman_to_utf8(zst::byte_span str);
}

namespace font
{
	// these are all BIG ENDIAN, because FUCK YOU.
	uint8_t peek_u8(const zst::byte_span& s)
	{
		assert(s.size() >= 1);
		return s[0];
	}

	uint16_t peek_u16(const zst::byte_span& s)
	{
		assert(s.size() >= 2);
		return (uint16_t) (((uint16_t) s[0] << (uint16_t) 8) | ((uint16_t) s[1] << (uint16_t) 0));
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

	uint64_t peek_u64(const zst::byte_span& s)
	{
		assert(s.size() >= 8);
		return ((uint64_t) s[0] << 56) //
		     | ((uint64_t) s[1] << 48) //
		     | ((uint64_t) s[2] << 40) //
		     | ((uint64_t) s[3] << 32) //
		     | ((uint64_t) s[4] << 24) //
		     | ((uint64_t) s[5] << 16) //
		     | ((uint64_t) s[6] << 8)  //
		     | ((uint64_t) s[7] << 0);
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

	uint64_t consume_u64(zst::byte_span& s)
	{
		auto ret = peek_u64(s);
		s.remove_prefix(8);
		return ret;
	}


	std::optional<std::string> convert_name_to_utf8(uint16_t platform_id, uint16_t encoding_id, zst::byte_span str)
	{
		// windows and unicode both use UTF-16BE
		if(platform_id == 0 || platform_id == 3)
		{
			return unicode::utf8FromUtf16BigEndianBytes(str);
		}
		else if(platform_id == 1)
		{
			// mac roman encoding
			if(encoding_id == 0)
				return misc::convert_mac_roman_to_utf8(str);
		}

		return std::nullopt;
	}




	static FontNames parse_name_table(zst::byte_span buf, const Table& name_table)
	{
		FontNames names {};
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
			auto text = convert_name_to_utf8(nr.platform_id, nr.encoding_id, storage.drop(nr.offset).take(nr.length));
			if(not text.has_value())
				continue;

			if(nr.name_id == 0)
				names.copyright_info = std::move(*text);
			else if(nr.name_id == 1)
				names.family_compat = std::move(*text);
			else if(nr.name_id == 2)
				names.subfamily_compat = std::move(*text);
			else if(nr.name_id == 3)
				names.unique_name = std::move(*text);
			else if(nr.name_id == 4)
				names.full_name = std::move(*text);
			else if(nr.name_id == 6)
				names.postscript_name = std::move(*text);
			else if(nr.name_id == 13)
				names.license_info = std::move(*text);
			else if(nr.name_id == 16)
				names.family = std::move(*text);
			else if(nr.name_id == 17)
				names.subfamily = std::move(*text);
			else
				names.others[nr.name_id] = std::move(*text);
		}

		if(names.family.empty())
			names.family = names.family_compat;

		if(names.subfamily.empty())
			names.subfamily = names.subfamily_compat;

		if(names.postscript_name.empty())
		{
			// TODO: need to remove spaces here
			zpr::fprintln(stderr, "warning: making up a postscript name, using '{}'", names.unique_name);
			names.postscript_name = names.unique_name;
		}

		return names;
	}

	void FontFile::parse_name_table(const Table& name_table)
	{
		m_names = ::font::parse_name_table(this->bytes(), name_table);
	}


	void FontFile::parse_cmap_table(const Table& cmap_table)
	{
		auto buf = this->bytes();
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
		m_character_mapping = readCMapTable(foo);
	}




	void FontFile::parse_head_table(const Table& head_table)
	{
		auto buf = this->bytes();
		buf.remove_prefix(head_table.offset);

		// skip the following: major ver (16), minor ver (16), font rev (32), checksum (32), magicnumber (32),
		// flags (16), unitsperem (16), created (64), modified (64).
		buf.remove_prefix(18);

		m_metrics.units_per_em = consume_u16(buf);

		buf.remove_prefix(16);

		m_metrics.xmin = consume_i16(buf);
		m_metrics.ymin = consume_i16(buf);
		m_metrics.xmax = consume_i16(buf);
		m_metrics.ymax = consume_i16(buf);

		// skip some stuff
		buf.remove_prefix(6);

		// by the time we reach this place, the tt_data should have been created
		// by the `glyf` table already.
		size_t loca_bytes = (consume_u16(buf) == 0) ? 2 : 4;
		if(m_outline_type == OUTLINES_TRUETYPE)
		{
			assert(m_truetype_data != nullptr);
			m_truetype_data->loca_bytes_per_entry = loca_bytes;
		}
	}

	void FontFile::parse_post_table(const Table& post_table)
	{
		auto buf = this->bytes();
		buf.remove_prefix(post_table.offset);

		// for now we only need the italic angle and the "isfixedpitch" flag.
		auto version = consume_u32(buf);
		(void) version;

		{
			auto whole = consume_i16(buf);
			auto frac = consume_u16(buf);

			m_metrics.italic_angle = static_cast<double>(whole) + (static_cast<double>(frac) / 65536.0);
		}

		consume_u32(buf); // skip the two underline-related metrics
		m_metrics.is_monospaced = consume_u32(buf) != 0;
	}

	void FontFile::parse_hhea_table(const Table& hhea_table)
	{
		auto buf = this->bytes();
		buf.remove_prefix(hhea_table.offset);

		// skip the versions
		buf.remove_prefix(4);

		// we should have already parsed the head table, so this value should be present.
		assert(m_metrics.units_per_em != 0);

		m_metrics.hhea_ascent = FontScalar(consume_i16(buf));
		m_metrics.hhea_descent = FontScalar(consume_i16(buf));
		m_metrics.hhea_linegap = FontScalar(consume_i16(buf));

		// skip all the nonsense
		buf.remove_prefix(24);

		m_num_hmetrics = consume_u16(buf);
	}

	void FontFile::parse_os2_table(const Table& os2_table)
	{
		auto buf = this->bytes();
		buf.remove_prefix(os2_table.offset);

		auto version = peek_u16(buf);

		// the sTypo things are always available, even at version 0
		m_metrics.typo_ascent = FontScalar(peek_i16(buf.drop(68)));
		m_metrics.typo_descent = FontScalar(peek_i16(buf.drop(70)));
		m_metrics.typo_linegap = FontScalar(peek_i16(buf.drop(72)));

		// "it is strongly recommended to use OS/2.sTypoAscender - OS/2.sTypoDescender+ OS/2.sTypoLineGap
		// as a value for default line spacing for this font."

		// note that FreeType also does this x1.2 "magic factor", and it seems to line up with
		// what other programs (eg. word, pages) use.
		m_metrics.default_line_spacing = std::max( //
		    FontScalar(m_metrics.units_per_em * 12) / 10,
		    m_metrics.typo_ascent - m_metrics.typo_descent + m_metrics.typo_linegap //
		);

		if(version >= 2)
		{
			m_metrics.x_height = FontScalar(peek_i16(buf.drop(84)));
			m_metrics.cap_height = FontScalar(peek_i16(buf.drop(84 + 2)));
		}
	}

	void FontFile::parse_hmtx_table(const Table& hmtx_table)
	{
		// hhea should have been parsed already, so this should be present.
		assert(m_num_hmetrics > 0);
		m_hmtx_table = this->bytes().drop(hmtx_table.offset).take(hmtx_table.length);

		// deferred processing -- we only take the glyph data when it is requested.
	}

	void FontFile::parse_maxp_table(const Table& maxp_table)
	{
		// we are only interested in the number of glyphs, so the version is
		// not really relevant (neither is any other field)
		auto buf = this->bytes();
		buf.remove_prefix(maxp_table.offset);

		auto version = consume_u32(buf);
		(void) version;

		m_num_glyphs = consume_u16(buf);
	}

	void FontFile::parse_cff_table(const Table& cff_table)
	{
		if(m_outline_type != FontFile::OUTLINES_CFF)
			sap::internal_error("found 'CFF' table in file with non-CFF outlines");

		auto cff_data = this->bytes().drop(cff_table.offset).take(cff_table.length);

		m_cff_data = cff::parseCFFData(this, cff_data);
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


	static std::map<Tag, Table> get_table_offsets(zst::byte_span buf)
	{
		std::map<Tag, Table> tables {};

		// first get the version.
		auto sfnt_version = Tag(consume_u32(buf));
		(void) sfnt_version;

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
			tables.emplace(tbl.tag, tbl);
		}

		return tables;
	}

	std::unique_ptr<FontFile> FontFile::from_offset_table(zst::unique_span<uint8_t[]> file_buf,
	    size_t start_of_offset_table)
	{
		// this is perfectly fine, because we own the data referred to by 'buf'.
		auto font = std::unique_ptr<FontFile>(new FontFile(std::move(file_buf)));
		auto file_bytes = font->bytes();

		font->m_tables = get_table_offsets(file_bytes.drop(start_of_offset_table));

		// first get the version.
		auto sfnt_version = Tag(peek_u32(file_bytes.drop(start_of_offset_table)));

		if(sfnt_version == Tag("OTTO"))
			font->m_outline_type = FontFile::OUTLINES_CFF;
		else if(sfnt_version == Tag(0, 1, 0, 0) || sfnt_version == Tag("true"))
			font->m_outline_type = FontFile::OUTLINES_TRUETYPE;
		else
			sap::internal_error("unsupported ttf/otf file; unknown header bytes '{}'", sfnt_version.str());


		// there is an order that we want to use:
		constexpr Tag table_processing_order[] = {
			Tag("head"),
			Tag("name"),
			Tag("hhea"),
			Tag("hmtx"),
			Tag("maxp"),
			Tag("post"),
			Tag("CFF "),
			Tag("CFF2"),
			Tag("glyf"),
			Tag("loca"),
			Tag("cmap"),
			Tag("GPOS"),
			Tag("GSUB"),
			Tag("kern"),
			Tag("morx"),
			Tag("OS/2"),
		};

		// CFF makes its own data (since everything is self-contained in the CFF table)
		// but for TrueType, it's split across several tables, so just make one here.
		if(font->m_outline_type == FontFile::OUTLINES_TRUETYPE)
			font->m_truetype_data = std::make_unique<truetype::TTData>();

		for(auto tag : table_processing_order)
		{
			if(auto it = font->sfntTables().find(tag); it != font->sfntTables().end())
			{
				auto& tbl = it->second;
				if(tag == Tag("CFF "))
					font->parse_cff_table(tbl);
				else if(tag == Tag("CFF2"))
					font->parse_cff_table(tbl);
				else if(tag == Tag("GPOS"))
					font->parse_gpos_table(tbl);
				else if(tag == Tag("GSUB"))
					font->parse_gsub_table(tbl);
				else if(tag == Tag("OS/2"))
					font->parse_os2_table(tbl);
				else if(tag == Tag("cmap"))
					font->parse_cmap_table(tbl);
				else if(tag == Tag("glyf"))
					font->parse_glyf_table(tbl);
				else if(tag == Tag("head"))
					font->parse_head_table(tbl);
				else if(tag == Tag("hhea"))
					font->parse_hhea_table(tbl);
				else if(tag == Tag("hmtx"))
					font->parse_hmtx_table(tbl);
				else if(tag == Tag("loca"))
					font->parse_loca_table(tbl);
				else if(tag == Tag("maxp"))
					font->parse_maxp_table(tbl);
				else if(tag == Tag("name"))
					font->parse_name_table(tbl);
				else if(tag == Tag("post"))
					font->parse_post_table(tbl);
				else if(tag == Tag("kern"))
					font->parse_kern_table(tbl);
				else if(tag == Tag("morx"))
					font->parse_morx_table(tbl);
			}
		}

		return font;
	}

	std::optional<std::unique_ptr<FontFile>> FontFile::from_postscript_name_in_collection( //
	    zst::unique_span<uint8_t[]> file_buf,
	    zst::str_view postscript_name)
	{
		assert(memcmp(file_buf.get(), "ttcf", 4) == 0);

		auto file_span = zst::byte_span(file_buf.get(), file_buf.size());

		// copy the thing
		auto ttc_span = file_span;

		// tag + major version + minor version
		ttc_span.remove_prefix(4 + 2 + 2);

		auto num_fonts = consume_u32(ttc_span);
		for(uint32_t i = 0; i < num_fonts; i++)
		{
			auto offset = consume_u32(ttc_span);
			auto tables = get_table_offsets(file_span.drop(offset));

			const auto& name_table = tables[Tag("name")];
			auto font_names = ::font::parse_name_table(file_span, name_table);

			if(font_names.postscript_name == postscript_name)
				return FontFile::from_offset_table(std::move(file_buf), offset);
		}

		return std::nullopt;
	}

	std::optional<std::unique_ptr<FontFile>> FontFile::fromHandle(FontHandle handle)
	{
		auto file = util::readEntireFile(handle.path);
		if(file.size() < 4)
			sap::internal_error("font file too short");

		auto span = zst::byte_span(file.get(), file.size());

		if(memcmp(span.data(), "OTTO", 4) == 0 || memcmp(span.data(), "true", 4) == 0
		    || memcmp(span.data(), "\x00\x01\x00\x00", 4) == 0)
		{
			return FontFile::from_offset_table(std::move(file), /* offset: */ 0);
		}
		else if(memcmp(span.data(), "ttcf", 4) == 0)
		{
			return FontFile::from_postscript_name_in_collection(std::move(file), handle.postscript_name);
		}
		else
		{
			sap::internal_error("unsupported font file; unknown header bytes '{}'", span.take(4).chars());
		}

		return std::nullopt;
	}

	FontFile::FontFile(zst::unique_span<uint8_t[]> bytes) : m_file(std::move(bytes))
	{
	}
}
