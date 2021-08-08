// subset.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "util.h"
#include "error.h"
#include "font/font.h"

namespace font
{
	static bool should_exclude_table(const Tag& tag)
	{
		return tag == Tag("GPOS")
			|| tag == Tag("GSUB")
			|| tag == Tag("GDEF")
			|| tag == Tag("BASE")
			|| tag == Tag("FFTM");

		// TODO: the OFF spec says that the 'post' table is required, but pdfTeX appears
		// to strip that table when it makes a subset...
	}

	static void write_num_tables_and_other_stuff(zst::byte_buffer& buf, size_t num_tables)
	{
		buf.append_bytes(util::convertBEU16(num_tables));

		// searchRange = "(Maximum power of 2 <= numTables) x 16"
		size_t max_pow2 = 0;
		while(max_pow2 < num_tables)
			max_pow2++;

		buf.append_bytes(util::convertBEU16((1 << max_pow2) * 16));

		// entrySelector = "Log2(Maximum power of 2 <= numTables)"
		buf.append_bytes(util::convertBEU16(max_pow2));

		// rangeShift = "NumTables x 16 - searchRange"
		buf.append_bytes(util::convertBEU16(16 * (num_tables - (1 << max_pow2))));
	}

	// TODO: this should take in the glyphs and stuff. for now we just strip
	// off the unnecessary tables.
	zst::byte_buffer createFontSubset(FontFile* font)
	{
		zst::byte_buffer buffer { };

		// it's easier to keep a list of which tables we want to keep, so we at least
		// know how many there are because we need to compute the offsets.
		std::vector<Table> included_tables;
		for(auto& [ _, table ] : font->tables)
		{
			// for now, use a blacklist. too lazy to figure out what each table does.
			if(!should_exclude_table(table.tag))
				included_tables.push_back(table);
		}

		if(font->outline_type == FontFile::OUTLINES_TRUETYPE)
			buffer.append_bytes(util::convertBEU32(0x00010000));

		else if(font->outline_type == FontFile::OUTLINES_CFF)
			buffer.append_bytes(util::convertBEU32(0x4F54544F));

		else
			sap::internal_error("unsupported outline type in font file");

		// num tables and some searching stuff
		write_num_tables_and_other_stuff(buffer, included_tables.size());

		static constexpr auto TableRecordSize = sizeof(uint32_t) * 4;

		// not exactly the current offset; this includes the size of the header
		// and of *all* the table records. makes writing the table offset easy.
		size_t current_table_offset = sizeof(uint32_t)
			+ (4 * sizeof(uint16_t))
			+ included_tables.size() * TableRecordSize;

		for(auto& table : included_tables)
		{
			buffer.append_bytes(util::convertBEU32(table.tag.value));
			buffer.append_bytes(util::convertBEU32(table.checksum));
			buffer.append_bytes(util::convertBEU32(current_table_offset));
			buffer.append_bytes(util::convertBEU32(table.length));

			current_table_offset += table.length;
		}

		auto file_contents = zst::byte_span(font->file_bytes, font->file_size);
		for(auto& table : included_tables)
			buffer.append(file_contents.drop(table.offset).take(table.length));

		return buffer;
	}
}
