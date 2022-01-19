// subset.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"
#include "error.h"

#include "font/cff.h"
#include "font/font.h"

namespace font::cff
{
	zst::byte_buffer createCFFSubset(FontFile* font, zst::str_view subset_name,
		const std::map<uint32_t, GlyphMetrics>& used_glyphs)
	{
		auto cff = font->cff_data;
		assert(cff != nullptr);

		zst::byte_buffer buffer {};

		// we only support subsetting CFF1 data for now.
		if(cff->cff2)
		{
			buffer.append(cff->bytes);
			return buffer;
		}

		// write the header
		buffer.append(1);   // major
		buffer.append(0);   // minor
		buffer.append(4);   // hdrSize
		buffer.append(4);   // offSize (for now, always 4. I don't even know what this field is used for)

		// Name INDEX (there is only 1)
		IndexTableBuilder()
			.add(subset_name.bytes())
			.writeInto(buffer);

		/*
			Top DICT (INDEX + Data)

			this is actually quite a complicated affair, due to the use of global offsets.

			In the Top DICT, the following keys use absolute offsets:
			charset, Encoding, CharStrings, Private, FDArray, FDSelect

			What we currently do (which is quite hacky...) is to hardcode these specific keys
			and always write them as 4-byte integers (ie. taking 5 bytes). This allows us to
			compute the size without actually writing the dict out. Anyway, that's abstracted
			out (we just call computeSize()) and so it's fine. probably.

			The Private DICT only has relative offsets, so that's fine.
		*/
		auto top_dict = DictBuilder(cff->top_dict);
		auto top_dict_size = top_dict.computeSize();

		/*
			String INDEX

			TODO: optimise this by getting rid of all the useless strings (which ones?)
		*/
		zst::byte_buffer string_table {};
		IndexTableBuilder()
			.add(cff->string_table)
			.writeInto(string_table);

		/*
			Global Subrs INDEX

			TODO: remove unused subrs
		*/
		zst::byte_buffer global_subrs_table {};
		{
			auto builder = IndexTableBuilder();
			for(auto& subr : cff->global_subrs)
				builder.add(subr.charstring);

			builder.writeInto(global_subrs_table);
		}

		/*
			Now, the rest of the stuff.
			We just check which ones are present in the original Top DICT and add them.
		*/
		{
			zst::byte_buffer tmp_buffer {};
			auto current_abs_offset = buffer.size()
				+ top_dict_size
				+ string_table.size()
				+ global_subrs_table.size();

			auto copy_kv_pair_with_abs_offset = [&](DictKey key, size_t size) {
				if(!cff->top_dict.contains(key))
					return;

				// Private needs special treatment (since it needs both a size and offset)
				if(key == DictKey::Private)
				{
					top_dict.setIntegerPair(key, size, current_abs_offset);
				}
				else
				{
					top_dict.setInteger(key, current_abs_offset);
				}

				current_abs_offset += size;
			};
		}



		// IndexTableBuilder()
		// 	.add(top_dict.serialise().span())
		// 	.writeInto(buffer);



		{
			auto f = fopen("kekw.cff", "wb");
			fwrite(buffer.data(), 1, buffer.size(), f);
			fclose(f);
		}

		return buffer;
	}
}
