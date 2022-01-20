// cff.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pool.h"
#include "font/cff.h"
#include "font/font.h"

namespace font::cff
{
	static IndexTable read_index_table(zst::byte_span& buf)
	{
		IndexTable index {};

		// if the count is 0, then no other data follows.
		index.count = consume_u16(buf);
		if(index.count == 0)
			return index;

		index.offset_bytes = consume_u8(buf);

		// there should be `count+1` entries, because the last one tells us the
		// size of the last data object
		for(uint16_t i = 0; i < index.count + 1; i++)
		{
			// the offset is from the last byte of the index data itself; since we are
			// consuming the entire INDEX structure, subtract 1 to get the "real" offset
			// from the start of the final buffer.
			if(index.offset_bytes == 1)
			{
				index.offsets.push_back(consume_u8(buf) - 1);
			}
			else if(index.offset_bytes == 2)
			{
				index.offsets.push_back(consume_u16(buf) - 1);
			}
			else if(index.offset_bytes == 3)
			{
				// what a pain...
				uint32_t b0 = consume_u8(buf);
				uint32_t b1 = consume_u8(buf);
				uint32_t b2 = consume_u8(buf);

				index.offsets.push_back(((b0 << 16) | (b1 << 8) | (b2)) - 1);
			}
			else if(index.offset_bytes == 4)
			{
				index.offsets.push_back(consume_u32(buf) - 1);
			}
			else
			{
				sap::error("font/cff", "unsupported offset size '{}'", index.offset_bytes);
			}
		}

		index.data = buf.take(index.offsets.back());

		assert(index.count + 1 == index.offsets.size());
		return index;
	}

	static std::vector<Subroutine> read_subrs_from_index(const IndexTable& index)
	{
		std::vector<Subroutine> subrs {};
		for(size_t i = 0; i < index.count; i++)
		{
			auto data = index.get_item(i);

			Subroutine subr {};
			subr.charstring = data;
			subr.subr_number = i;
			subr.used = false;
			subrs.push_back(std::move(subr));
		}
		return subrs;
	}



	CFFData* parseCFFData(FontFile* font, zst::byte_span buf)
	{
		auto cff = util::make<CFFData>();
		cff->bytes = buf;

		/*
			the first 5 items are in a fixed order:
			Header
			Name (INDEX + Data)
			Top DICT (INDEX + Data),
			Strings (INDEX + Data),
			Global Subrs INDEX
		*/

		// first, the header
		{
			auto a = consume_u8(buf);
			auto b = consume_u8(buf);

			// TODO: we might not have parsed the name information yet!
			if(a != 1 && a != 2)
			{
				sap::error("font/cff", "font '{}' has unsupported CFF version '{}.{}'",
					font->full_name, a, b);
			}
			else if(b != 0)
			{
				sap::warn("font/cff", "font '{}' has unsupported CFF version '{}.{}' (expected minor 0)",
					font->full_name, a, b);
			}

			if(a == 2)
				cff->cff2 = true;

			// now comes the header size
			auto hdr_size = consume_u8(buf);
			consume_u8(buf);

			// remove any extra bytes of the header; we already read 4
			buf.remove_prefix(hdr_size - 4);
		}

		// now, the Name INDEX. this is just a list of fonts
		{
			auto name_index = read_index_table(buf);

			// CFF fonts embedded in OTFs must have exactly 1 font (ie. no font sets/collections)
			if(name_index.count != 1)
				sap::error("font/cff", "invalid number of entries in Name INDEX (expected 1, got {})", name_index.count);

			cff->name = name_index.get_item(0);

			// now skip the data
			buf.remove_prefix(name_index.data.size());
		}

		// Top DICT
		{
			auto dict_index = read_index_table(buf);
			if(dict_index.count != 1)
				sap::error("font/cff", "invalid number of entries in Top DICT INDEX (expected 1, got {})", dict_index.count);

			// top dict needs to be populated with default values as well.
			cff->top_dict = readDictionary(dict_index.data);
			populateDefaultValuesForTopDict(cff->top_dict);

			if(cff->top_dict.contains(DictKey::ROS))
				cff->is_cidfont = true;

			if(auto foo = cff->top_dict.integer(DictKey::CharstringType); foo != 2)
				sap::error("font/cff", "unsupported charstring type '{}', expected '2'", foo);

			buf.remove_prefix(dict_index.data.size());

			// read the charstrings index from the top dict
			{
				if(!cff->top_dict.contains(DictKey::CharStrings))
					sap::error("font/cff", "Top DICT missing CharStrings key");

				// specified from the beginning of the file
				auto charstrings_offset = cff->top_dict.integer(DictKey::CharStrings);
				auto tmp = cff->bytes.drop(charstrings_offset);
				cff->charstrings_table = read_index_table(tmp);

				if(cff->charstrings_table.count == 0)
					sap::error("font/cff", "font contains no glyphs!");
			}

			if(cff->top_dict.contains(DictKey::FDArray))
			{
				auto fdarray_ofs = cff->top_dict.integer(DictKey::Encoding);
				auto fdarray_data = cff->bytes.drop(fdarray_ofs);
				cff->fdarray_table = read_index_table(fdarray_data);
			}

			if(cff->top_dict.contains(DictKey::FDSelect))
			{
				auto fdselect_ofs = cff->top_dict.integer(DictKey::FDSelect);

				auto tmp_data = cff->bytes.drop(fdselect_ofs);
				auto format = tmp_data[0];
				tmp_data.remove_prefix(1);

				if(format == 0)
				{
					for(size_t i = 0; i < cff->charstrings_table.count; i++)
						consume_u16(tmp_data);
				}
				else if(format == 3)
				{
					auto num_ranges = consume_u16(tmp_data);
					for(size_t i = 0; i < num_ranges; i++)
						tmp_data.remove_prefix(3);

					tmp_data.remove_prefix(2);
				}
				else
				{
					sap::error("font/cff", "unsupported FDSelect format '{}' (expected 0 or 3)", format);
				}

				auto fdselect_data = cff->bytes.drop(fdselect_ofs);
				auto fdselect_size = fdselect_data.size() - tmp_data.size();

				cff->fdselect_data = fdselect_data.take(fdselect_size);
			}
		}


		// String INDEX
		auto string_table = read_index_table(buf);
		buf.remove_prefix(string_table.data.size());


		// Global Subrs INDEX
		{
			auto index = read_index_table(buf);
			cff->global_subrs = read_subrs_from_index(index);

			buf.remove_prefix(index.data.size());
		}

		// read the local subrs from the private dict
		{
			auto foo = cff->top_dict.get(DictKey::Private);
			if(foo.size() != 2)
				sap::error("font/cff", "missing Private DICT");

			auto size = foo[0].integer();
			auto offset = foo[1].integer();

			// private dict offset is specified from the beginning of the file
			cff->private_dict = readDictionary(cff->bytes.drop(offset).take(size));

			// local subrs index is specified from the beginning of *the private DICT data)
			if(cff->private_dict.contains(DictKey::Subrs))
			{
				auto local_subr_offset = cff->private_dict.integer(DictKey::Subrs);
				local_subr_offset += offset;

				auto tmp = cff->bytes.drop(local_subr_offset);
				auto index = read_index_table(tmp);

				cff->local_subrs = read_subrs_from_index(index);
			}
		}

		// ensure we keep the copyright etc. strings available
		{
			auto add_string_if_needed = [&](DictKey key) {
				if(cff->top_dict.contains(key))
				{
					auto sid = cff->top_dict.string_id(key);
					auto foo = string_table.get_item(sid - NUM_STANDARD_STRINGS).cast<char>();
					cff->top_dict.values[key] = { Operand().string_id(cff->get_or_add_string(foo)) };
				}
			};

			add_string_if_needed(DictKey::version);
			add_string_if_needed(DictKey::Notice);
			add_string_if_needed(DictKey::Copyright);
			add_string_if_needed(DictKey::FullName);
			add_string_if_needed(DictKey::FamilyName);
			add_string_if_needed(DictKey::Weight);
		}






		// populate the glyph list.
		{
			auto charset_ofs = cff->top_dict.integer(DictKey::charset);

			std::map<uint16_t, uint16_t> charset_mapping {};

			if(charset_ofs == 0 || charset_ofs == 1 || charset_ofs == 2)
				charset_mapping = getPredefinedCharset(charset_ofs);
			else
				charset_mapping = readCharsetTable(cff->charstrings_table.count, cff->bytes.drop(charset_ofs));

			// charset-mapping maps from gid -> sid or cid, depending on whether this is a CID font or not
			for(auto& [ gid, sid ] : charset_mapping)
			{
				Glyph glyph {};
				glyph.gid = gid;
				glyph.charstring = cff->charstrings_table.get_item(gid);

				if(!cff->is_cidfont)
					glyph.glyph_name = string_table.get_item(sid - NUM_STANDARD_STRINGS).cast<char>();
				else
					glyph.cid = sid;

				cff->glyphs.push_back(std::move(glyph));
			}
		}



		return cff;
	}
}
