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
				index.offsets.push_back(consume_u8(buf) - 1);
			else if(index.offset_bytes == 2)
				index.offsets.push_back(consume_u16(buf) - 1);
			else if(index.offset_bytes == 4)
				index.offsets.push_back(consume_u32(buf) - 1);
			else
				sap::error("font/cff", "unsupported offset size '{}'", index.offset_bytes);
		}

		index.data = buf.take(index.offsets.back());

		assert(index.count + 1 == index.offsets.size());
		return index;
	}


	CFFData* parseCFFData(FontFile* font, zst::byte_span buf)
	{
		auto cff = util::make<CFFData>();
		cff->bytes = buf;

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

			// this isn't too important
			// zpr::println("name = {}", name_data.cast<char>());

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
		}

		// String INDEX
		{
			cff->string_table = read_index_table(buf);

			zpr::println("notice = '{}'", cff->get_string(cff->top_dict.string_id(DictKey::version)));
		}



		return cff;
	}

}
