// cff.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pool.h"
#include "font/cff.h"
#include "font/font.h"

namespace font::cff
{
	IndexTable readIndexTable(zst::byte_span buf, size_t* total_size)
	{
		IndexTable index {};

		auto start = buf;

		// if the count is 0, then no other data follows.
		index.count = consume_u16(buf);
		if(index.count == 0)
		{
			if(total_size)
				*total_size = 2;
			return index;
		}

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
		if(total_size)
			*total_size = (start.size() - buf.size()) + index.data.size();

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
				sap::error("font/cff", "font '{}' has unsupported CFF version '{}.{}'", font->full_name, a, b);
			}
			else if(b != 0)
			{
				sap::warn("font/cff", "font '{}' has unsupported CFF version '{}.{}' (expected minor 0)", font->full_name, a, b);
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
			size_t size = 0;
			auto name_index = readIndexTable(buf, &size);
			buf.remove_prefix(size);

			// CFF fonts embedded in OTFs must have exactly 1 font (ie. no font sets/collections)
			if(name_index.count != 1)
				sap::error("font/cff", "invalid number of entries in Name INDEX (expected 1, got {})", name_index.count);

			cff->name = name_index.get_item(0);
		}



		// Top DICT
		{
			size_t size = 0;
			auto dict_index = readIndexTable(buf, &size);
			buf.remove_prefix(size);

			if(dict_index.count != 1)
				sap::error("font/cff", "invalid number of entries in Top DICT INDEX (expected 1, got {})", dict_index.count);

			// top dict needs to be populated with default values as well.
			cff->top_dict = readDictionary(dict_index.data);
			populateDefaultValuesForTopDict(cff->top_dict);

			if(cff->top_dict.contains(DictKey::ROS))
				cff->is_cidfont = true;

			if(auto foo = cff->top_dict.integer(DictKey::CharstringType); foo != 2)
				sap::error("font/cff", "unsupported charstring type '{}', expected '2'", foo);



			// read the charstrings index from the top dict
			if(!cff->top_dict.contains(DictKey::CharStrings))
				sap::error("font/cff", "Top DICT missing CharStrings key");

			// specified from the beginning of the file
			auto charstrings_offset = cff->top_dict.integer(DictKey::CharStrings);
			cff->charstrings_table = readIndexTable(cff->bytes.drop(charstrings_offset));

			if(cff->charstrings_table.count == 0)
				sap::error("font/cff", "font contains no glyphs!");
		}


		// String INDEX
		size_t string_table_size = 0;
		auto string_table = readIndexTable(buf, &string_table_size);
		buf.remove_prefix(string_table_size);


		// Global Subrs INDEX
		{
			size_t size = 0;
			auto index = readIndexTable(buf, &size);
			buf.remove_prefix(size);

			cff->global_subrs = read_subrs_from_index(index);
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
			for(auto& [gid, sid] : charset_mapping)
			{
				Glyph glyph {};
				glyph.gid = gid;
				glyph.charstring = cff->charstrings_table.get_item(gid);
				glyph.font_dict_idx = 0;

				if(!cff->is_cidfont)
					glyph.glyph_name = string_table.get_item(sid - NUM_STANDARD_STRINGS).cast<char>();
				else
					glyph.cid = sid;

				cff->glyphs.push_back(std::move(glyph));
			}
		}

		auto read_private_dict_and_local_subrs_from_dict = [cff](const Dictionary& dict) -> auto
		{
			auto foo = dict.get(DictKey::Private);
			if(foo.size() != 2)
				sap::error("font/cff", "missing Private DICT");

			auto size = foo[0].integer();
			auto offset = foo[1].integer();

			// private dict offset is specified from the beginning of the file
			auto private_dict = readDictionary(cff->bytes.drop(offset).take(size));
			std::vector<Subroutine> local_subrs {};

			// local subrs index is specified from the beginning of the private DICT data)
			if(private_dict.contains(DictKey::Subrs))
			{
				auto local_subr_offset = private_dict.integer(DictKey::Subrs);
				local_subr_offset += offset;

				auto index = readIndexTable(cff->bytes.drop(local_subr_offset));
				local_subrs = read_subrs_from_index(index);
			}

			return std::pair(std::move(private_dict), std::move(local_subrs));
		};


		if(!cff->is_cidfont)
		{
			auto [private_dict, local_subrs] = read_private_dict_and_local_subrs_from_dict(cff->top_dict);

			FontDict fontdict {};
			fontdict.private_dict = std::move(private_dict);
			fontdict.local_subrs = std::move(local_subrs);

			cff->font_dicts.push_back(std::move(fontdict));
		}
		else
		{
			auto fdarray_ofs = cff->top_dict.integer(DictKey::FDArray);
			auto fdarray_table = readIndexTable(cff->bytes.drop(fdarray_ofs));

			for(auto i = 0; i < fdarray_table.count; i++)
			{
				FontDict fd {};
				fd.dict = readDictionary(fdarray_table.get_item(i));

				auto [private_dict, local_subrs] = read_private_dict_and_local_subrs_from_dict(fd.dict);
				fd.private_dict = std::move(private_dict);
				fd.local_subrs = std::move(local_subrs);

				cff->font_dicts.push_back(std::move(fd));
			}


			auto fdselect_ofs = cff->top_dict.integer(DictKey::FDSelect);
			auto fdselect_data = cff->bytes.drop(fdselect_ofs);

			auto format = fdselect_data[0];
			fdselect_data.remove_prefix(1);

			if(format == 0)
			{
				for(size_t i = 0; i < cff->glyphs.size(); i++)
					cff->glyphs[i].font_dict_idx = consume_u8(fdselect_data);
			}
			else if(format == 3)
			{
				auto num_ranges = consume_u16(fdselect_data);
				auto last_gid = peek_u16(fdselect_data.drop(num_ranges * 3));

				for(size_t i = 0; i < num_ranges; i++)
				{
					auto first = consume_u16(fdselect_data);
					auto fd = consume_u8(fdselect_data);

					auto last = (i + 1 == num_ranges) ? last_gid : peek_u16(fdselect_data);

					for(auto gid = first; gid < last; gid++)
						cff->glyphs[gid].font_dict_idx = fd;
				}

				fdselect_data.remove_prefix(2);
			}
			else
			{
				sap::error("font/cff", "unsupported FDSelect format '{}' (expected 0 or 3)", format);
			}
		}


		return cff;
	}
}
