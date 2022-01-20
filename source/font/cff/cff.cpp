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
			cff->absolute_offset_bytes = consume_u8(buf);

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
				cff->num_glyphs = cff->charstrings_table.count;

				if(cff->num_glyphs == 0)
					sap::error("font/cff", "font contains no glyphs!");
			}


			if(cff->top_dict.contains(DictKey::Encoding))
			{
				// get the absolute offset to the encoding data. if the offset is 0 or 1, then it's
				// a predefined (builtin) encoding, and doesn't contain any data.
				auto encoding_ofs = cff->top_dict.integer(DictKey::Encoding);
				if(encoding_ofs != 0 && encoding_ofs != 1)
				{
					auto encoding_data = cff->bytes.drop(encoding_ofs);
					size_t encoding_size = 1;

					// now we must calculate the size.
					auto format = encoding_data[0] & 0x7F;
					bool have_supplements = encoding_data[0] & 0x80;

					if(format == 0)
					{
						auto num_codes = encoding_data[1];
						encoding_size += (1 + num_codes);
					}
					else if(format == 1)
					{
						auto num_ranges = encoding_data[1];
						encoding_size += (2 * num_ranges) + 1;
					}
					else
					{
						sap::error("font/cff", "unsupported Encodings format '{}' (expected 0 or 1)", format);
					}

					if(have_supplements)
					{
						auto tmp = encoding_data.drop(encoding_size);
						auto num_sups = tmp[0];
						tmp.remove_prefix(1);
						encoding_size += 1;

						// this contains SIDs, so we are forced to read it
						while(num_sups > 0)
						{
							tmp.remove_prefix(1);   // code (Card8)

							auto foo = tmp.size();
							consume_u16(tmp);
							encoding_size += 1 + (foo - tmp.size());

							num_sups--;
						}
					}

					cff->encoding_data = encoding_data.take(encoding_size);
				}
			}

			if(cff->top_dict.contains(DictKey::charset))
			{
				auto charset_ofs = cff->top_dict.integer(DictKey::charset);
				// 0 = ISOAdobe, 1 = Expert, 2 = ExpertSubset. Skip those (they don't take up space)
				if(charset_ofs != 0 && charset_ofs != 1 && charset_ofs != 2)
				{
					auto tmp_data = cff->bytes.drop(charset_ofs);
					auto format = tmp_data[0];
					tmp_data.remove_prefix(1);

					if(format == 0)
					{
						// sids... and num_glyphs - 1 sids...
						for(size_t i = 1; i < cff->num_glyphs; i++)
							consume_u16(tmp_data);
					}
					else if(format == 1 || format == 2)
					{
						// fuck you, adobe. "The number of ranges is not explicitly specified in the font.
						// Instead, software utilising this data simply processes ranges until all glyphs
						// in the font are covered"
						// stupid design, absolutely garbage.
						size_t required_glyphs = cff->num_glyphs - 1;
						while(required_glyphs > 0)
						{
							// read the sid.
							consume_u16(tmp_data);

							// read the nLeft (1 byte if format == 1, 2 if format == 2)
							if(format == 1)
								required_glyphs -= (1 + consume_u8(tmp_data));
							else
								required_glyphs -= (1 + consume_u16(tmp_data));
						}
					}
					else
					{
						sap::error("font/cff", "unsupported charset format '{}' (expected 0, 1, or 2)", format);
					}

					auto charset_data = cff->bytes.drop(charset_ofs);
					auto charset_size = charset_data.size() - tmp_data.size();

					cff->charset_data = charset_data.take(charset_size);
					zpr::println("charset size = {}", charset_size);
				}
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
					for(size_t i = 0; i < cff->num_glyphs; i++)
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
		{
			cff->string_table = read_index_table(buf);
			buf.remove_prefix(cff->string_table.data.size());
		}

		// Global Subrs INDEX
		{
			auto index = read_index_table(buf);
			cff->global_subrs = read_subrs_from_index(index);

			buf.remove_prefix(index.data.size());
		}

		// read the local subrs from the private dict
		{
			auto foo = cff->top_dict[DictKey::Private];
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

		return cff;
	}
}
