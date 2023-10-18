// subset.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <ctime> // for size_t, time

#include "util.h"  // for checked_cast, convertBEU32, convertBEU16
#include "types.h" // for GlyphId

#include "pdf/object.h" // for Stream

#include "font/cff.h"       // for CFFSubset, createCFFSubset
#include "font/tag.h"       // for Tag
#include "font/truetype.h"  // for TTSubset, createTTSubset
#include "font/font_file.h" // for Table, FontFile, Stream, FontFile::OUTLIN...

namespace font
{
	using pdf::Stream;

	static bool should_exclude_table(const Tag& tag)
	{
		return tag == Tag("GPOS") || tag == Tag("GSUB") || tag == Tag("GDEF") || tag == Tag("BASE")
		    || tag == Tag("FFTM");
	}

	static void write_num_tables_and_other_stuff(Stream* stream, size_t num_tables)
	{
		stream->append_bytes(util::convertBEU16(util::checked_cast<uint16_t>(num_tables)));

		// searchRange = "(Maximum power of 2 <= numTables) x 16"
		size_t max_pow2 = 0;
		while((1u << (1u + max_pow2)) < num_tables)
			max_pow2++;

		assert((1u << max_pow2) <= num_tables);

		stream->append_bytes(util::convertBEU16(util::checked_cast<uint16_t>(1 << max_pow2) * 16));

		// entrySelector = "Log2(Maximum power of 2 <= numTables)"
		stream->append_bytes(util::convertBEU16(util::checked_cast<uint16_t>(max_pow2)));

		// rangeShift = "NumTables x 16 - searchRange"
		stream->append_bytes(util::convertBEU16(util::checked_cast<uint16_t>(16 * (num_tables - (1 << max_pow2)))));
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
			last_u32 |= ((uint32_t) remaining[i] << (24 - (i * 8)));

		uint32_t checksum = 0;
		for(auto u32 : u32s)
			checksum += util::convertBEU32(u32);

		checksum += last_u32;
		return checksum;
	}


	void FontFile::writeSubset(zst::str_view subset_name, Stream* stream)
	{
		auto file_contents = this->bytes();

		// it's easier to keep a list of which tables we want to keep, so we at least
		// know how many there are because we need to compute the offsets.
		std::vector<Table> included_tables {};
		for(auto& [_, table] : m_tables)
		{
			// for now, use a blacklist. too lazy to figure out what each table does.
			if(!should_exclude_table(table.tag))
				included_tables.push_back(table);
		}

		if(this->hasTrueTypeOutlines())
			stream->append_bytes(util::convertBEU32(0x00010000));

		else if(this->hasCffOutlines())
			stream->append_bytes(util::convertBEU32(0x4F54544F));

		else
			sap::internal_error("unsupported outline type in font file");

		// num tables and some searching stuff
		write_num_tables_and_other_stuff(stream, included_tables.size());

		static constexpr auto TableRecordSize = sizeof(uint32_t) * 4;

		// not exactly the current offset; this includes the size of the header
		// and of *all* the table records. makes writing the table offset easy.
		size_t
		    current_table_offset = sizeof(uint32_t) + (4 * sizeof(uint16_t)) + included_tables.size() * TableRecordSize;

		auto write_table_record = [&stream, &current_table_offset](const Tag& tag, uint32_t checksum, uint32_t size) {
			stream->append_bytes(util::convertBEU32(tag.value));
			stream->append_bytes(util::convertBEU32(checksum));
			stream->append_bytes(util::convertBEU32(util::checked_cast<uint32_t>(current_table_offset)));
			stream->append_bytes(util::convertBEU32(size));

			current_table_offset += size;
		};

		if(this->hasTrueTypeOutlines())
		{
			auto subset = this->createTTSubset();

			for(auto& table : included_tables)
			{
				size_t size = table.length;
				size_t checksum = table.checksum;

				// note: only need to check glyf -- loca should be the same size.
				if(table.tag == Tag("glyf"))
				{
					size = subset.glyf_table.size();
					checksum = compute_checksum(subset.glyf_table.span());
				}
				else if(table.tag == Tag("loca"))
				{
					size = subset.loca_table.size();
					checksum = compute_checksum(subset.loca_table.span());
				}

				write_table_record(table.tag, util::checked_cast<uint32_t>(checksum),
				    util::checked_cast<uint32_t>(size));
			}

			for(auto& table : included_tables)
			{
				if(table.tag == Tag("glyf"))
					stream->append(subset.glyf_table.span());
				else if(table.tag == Tag("loca"))
					stream->append(subset.loca_table.span());
				else
					stream->append(file_contents.drop(table.offset).take(table.length));
			}
		}
		else
		{
			auto cff_subset = this->createCFFSubset(subset_name);

			for(auto& table : included_tables)
			{
				size_t size = table.length;
				size_t checksum = table.checksum;
				if(table.tag == Tag("CFF ") || table.tag == Tag("CFF2"))
				{
					size = cff_subset.cff.size();
					checksum = compute_checksum(cff_subset.cff.span());
				}
				else if(table.tag == Tag("cmap"))
				{
					size = cff_subset.cmap.size();
					checksum = compute_checksum(cff_subset.cmap.span());
				}
				write_table_record(table.tag, util::checked_cast<uint32_t>(checksum),
				    util::checked_cast<uint32_t>(size));
			}

			for(auto& table : included_tables)
			{
				if(table.tag == Tag("CFF ") || table.tag == Tag("CFF2"))
					stream->append(cff_subset.cff.span());

				else if(table.tag == Tag("cmap"))
					stream->append(cff_subset.cmap.span());

				else
					stream->append(file_contents.drop(table.offset).take(table.length));
			}
		}
	}


	std::string generateSubsetName(const FontFile* font)
	{
		static auto foo = []() {
			srand(static_cast<unsigned int>(time(nullptr)));
			return 69;
		}();
		(void) foo;

		// does this really need to be very random? no.
		const char* letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

		return zpr::sprint("{}{}{}{}{}{}+{}", letters[rand() % 26], letters[rand() % 26], letters[rand() % 26],
		    letters[rand() % 26], letters[rand() % 26], letters[rand() % 26], font->names().postscript_name);
	}
}
