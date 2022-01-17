// subset.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cstdlib>

#include "util.h"
#include "error.h"
#include "font/font.h"
#include "pdf/object.h"

namespace font
{
	using pdf::Stream;

	static bool should_exclude_table(const Tag& tag)
	{
		return tag == Tag("GPOS")
			|| tag == Tag("GSUB")
			|| tag == Tag("GDEF")
			|| tag == Tag("BASE")
			|| tag == Tag("FFTM")
			|| tag == Tag("post");

		// TODO: the OFF spec says that the 'post' table is required, but pdfTeX appears
		// to strip that table when it makes a subset. for now we strip it also.
	}

	static void write_num_tables_and_other_stuff(Stream* stream, size_t num_tables)
	{
		stream->append_bytes(util::convertBEU16(num_tables));

		// searchRange = "(Maximum power of 2 <= numTables) x 16"
		size_t max_pow2 = 0;
		while(max_pow2 < num_tables)
			max_pow2++;

		stream->append_bytes(util::convertBEU16((1 << max_pow2) * 16));

		// entrySelector = "Log2(Maximum power of 2 <= numTables)"
		stream->append_bytes(util::convertBEU16(max_pow2));

		// rangeShift = "NumTables x 16 - searchRange"
		stream->append_bytes(util::convertBEU16(16 * (num_tables - (1 << max_pow2))));
	}

	// glyphId -> offset from start of glyf table
	using GlyfOffset = std::vector<size_t>;
	using GlyphIdMap = std::map<uint32_t, GlyphMetrics>;

	static zst::byte_buffer create_subset_glyf_table(FontFile* font, GlyfOffset& new_loca_table,
		const GlyfOffset& old_loca_table, const GlyphIdMap& used_glyphs, zst::byte_span old_glyf_table)
	{
		zst::byte_buffer buf { };

		// the loca table must contain an entry for every glyph id in the font. since we're not
		// changing the glyph ids themselves, we must iterate over every glyph id.
		for(size_t i = 0; i < font->num_glyphs; i++)
		{
			// every glyph must appear in the loca table. if it's not used then table_size is
			// not incremented, so it is all according to keikaku.
			new_loca_table.push_back(buf.size());

			if(i != 0 && used_glyphs.find(i) == used_glyphs.end())
				continue;

			buf.append(old_glyf_table.take(old_loca_table[i + 1]).drop(old_loca_table[i]));
		}

		// add the last entry.
		new_loca_table.push_back(buf.size());
		return buf;
	}

	static zst::byte_buffer create_subset_loca_table(FontFile* font, const GlyfOffset& new_loca_table)
	{
		zst::byte_buffer buf { };

		bool half = (font->loca_bytes_per_entry == 2);

		for(auto& ofs : new_loca_table)
		{
			if(half)    buf.append_bytes(util::convertBEU16(static_cast<uint16_t>(ofs / 2)));
			else        buf.append_bytes(util::convertBEU32(static_cast<uint32_t>(ofs)));
		}

		return buf;
	}

	static GlyfOffset read_loca_table(FontFile* font, zst::byte_span loca_table)
	{
		GlyfOffset glyf_offsets { };

		// note: we read num_glyphs + 1 because the loca table actually contains that
		// many entries; the last entry is the total size of the glyf table.
		for(size_t i = 0; i < font->num_glyphs + 1; i++)
		{
			size_t offset = 0;
			if(font->loca_bytes_per_entry == 2)
				offset = 2 * consume_u16(loca_table);
			else
				offset = consume_u32(loca_table);

			glyf_offsets.push_back(offset);
		}

		return glyf_offsets;
	}

	static uint32_t compute_checksum(zst::byte_span buf)
	{
		// tables must be 4 bytes-aligned and padded, but idk if we do that.
		auto u32s = buf.drop_last(buf.size() % sizeof(uint32_t)).cast<uint32_t>();
		auto remaining = buf.take_last(buf.size() % sizeof(uint32_t));

		// i don't know if this is right, but assume the bytes are big-endian, so
		// we must convert them to little-endian to sum, then write out the big-endian checksum.

		assert(remaining.size() < sizeof(uint32_t));
		uint32_t last_u32 = 0;
		for(size_t i = 0; i < remaining.size(); i++)
			last_u32 |= (remaining[i] << (24 - (i * 8)));

		uint32_t checksum = 0;
		for(auto u32 : u32s)
			checksum += util::convertBEU32(u32);

		checksum += last_u32;
		return checksum;
	}




	/*
		TODO(big oof)

		turns out that some glyphs are actually just copies of other glyphs (eg. fullwidth variants). So,
		just including the clone in the subset is not going to work if we don't also include its derivative;
		so we need to figure out how to find out wtf the base glyph actually is...

		(this only applies to fonts with truetype outlines -- we can't subset CFF outlines for now)

		ok, so the idea is that we need to actually inspect (on a surface level at least) the glyph data in
		the `glyf` table for the data, which tells us if it's a composite glyph, and if so which glyph(s) make
		up its components.
	*/
	void writeFontSubset(FontFile* font, Stream* stream, const std::map<uint32_t, GlyphMetrics>& used_glyphs)
	{
		auto file_contents = zst::byte_span(font->file_bytes, font->file_size);
		// stream->append(file_contents);
		// return;

		GlyfOffset old_loca_table { };
		zst::byte_span old_glyf_table;

		// it's easier to keep a list of which tables we want to keep, so we at least
		// know how many there are because we need to compute the offsets.
		std::vector<Table> included_tables;
		for(auto& [ _, table ] : font->tables)
		{
			// for now, use a blacklist. too lazy to figure out what each table does.
			if(!should_exclude_table(table.tag))
				included_tables.push_back(table);

			// by right, `loca` should only appear in ttf outline fonts.
			if(table.tag == Tag("loca"))
			{
				assert(font->outline_type == FontFile::OUTLINES_TRUETYPE);
				old_loca_table = read_loca_table(font, file_contents.drop(table.offset).take(table.length));
			}
			else if(table.tag == Tag("glyf"))
			{
				assert(font->outline_type == FontFile::OUTLINES_TRUETYPE);
				old_glyf_table = file_contents.drop(table.offset).take(table.length);
			}
		}

		if(font->outline_type == FontFile::OUTLINES_TRUETYPE)
			stream->append_bytes(util::convertBEU32(0x00010000));

		else if(font->outline_type == FontFile::OUTLINES_CFF)
			stream->append_bytes(util::convertBEU32(0x4F54544F));

		else
			sap::internal_error("unsupported outline type in font file");

		// num tables and some searching stuff
		write_num_tables_and_other_stuff(stream, included_tables.size());

		static constexpr auto TableRecordSize = sizeof(uint32_t) * 4;

		// not exactly the current offset; this includes the size of the header
		// and of *all* the table records. makes writing the table offset easy.
		size_t current_table_offset = sizeof(uint32_t)
			+ (4 * sizeof(uint16_t))
			+ included_tables.size() * TableRecordSize;

		auto write_table_record = [&stream, &current_table_offset](const Tag& tag, uint32_t checksum, uint32_t size) {
			stream->append_bytes(util::convertBEU32(tag.value));
			stream->append_bytes(util::convertBEU32(checksum));
			stream->append_bytes(util::convertBEU32(current_table_offset));
			stream->append_bytes(util::convertBEU32(size));

			current_table_offset += size;
		};

		if(font->outline_type == FontFile::OUTLINES_TRUETYPE)
		{
			// because we need to compute the dumb checksum (and it comes *before* the table itself)
			// we are forced to build a buffer beforehand, wasting memory & allocation & cpu cycles.
			GlyfOffset loca_mapping { };
			auto new_glyf_table = create_subset_glyf_table(font, loca_mapping, old_loca_table, used_glyphs, old_glyf_table);
			auto new_loca_table = create_subset_loca_table(font, loca_mapping);

			for(auto& table : included_tables)
			{
				size_t size = table.length;
				size_t checksum = table.checksum;

				// note: only need to check glyf -- loca should be the same size.
				if(table.tag == Tag("glyf"))
				{
					size = new_glyf_table.size();
					checksum = compute_checksum(new_glyf_table.span());
				}
				else if(table.tag == Tag("loca"))
				{
					size = new_loca_table.size();
					checksum = compute_checksum(new_loca_table.span());
				}

				write_table_record(table.tag, checksum, size);
			}

			for(auto& table : included_tables)
			{
				if(table.tag == Tag("glyf"))
					stream->append(new_glyf_table.span());
				else if(table.tag == Tag("loca"))
					stream->append(new_loca_table.span());
				else
					stream->append(file_contents.drop(table.offset).take(table.length));
			}
		}
		else
		{
			for(auto& table : included_tables)
				write_table_record(table.tag, table.checksum, table.length);

			for(auto& table : included_tables)
				stream->append(file_contents.drop(table.offset).take(table.length));
		}

		if(stream->is_compressed)
			stream->dict->add(pdf::names::Length1, pdf::Integer::create(stream->uncompressed_length));
	}


	std::string generateSubsetName(FontFile* font)
	{
		static auto foo = []() {
			srand(time(nullptr));
			return 69;
		}(); (void) foo;

		// does this really need to be very random? no.
		const char* letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

		return zpr::sprint("{}{}{}{}{}{}+{}",
			letters[rand() % 26], letters[rand() % 26], letters[rand() % 26],
			letters[rand() % 26], letters[rand() % 26], letters[rand() % 26],
			font->postscript_name);
	}
}
