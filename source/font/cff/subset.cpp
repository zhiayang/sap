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
			the Top DICT also has an INDEX before it, so we need to calculate the amount of space
			for the INDEX table as well.
		*/
		{
			int off_size = 0;
			if(top_dict_size < 255)         off_size = 1;
			else if(top_dict_size < 65535)  off_size = 2;
			else                            off_size = 4;

			top_dict_size += 2 + 1 + (2 * off_size);
		}




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
			const auto initial_abs_offset = buffer.size()
				+ top_dict_size
				+ string_table.size()
				+ global_subrs_table.size();

			// call this *BEFORE* writing the data!!!
			auto copy_kv_pair_with_abs_offset = [&](DictKey key, size_t size = 0) {
				if(!cff->top_dict.contains(key))
					return;

				// Private needs special treatment (since it needs both a size and offset)
				if(key == DictKey::Private)
				{
					top_dict.setIntegerPair(key, size, initial_abs_offset + tmp_buffer.size());
				}
				else
				{
					top_dict.setInteger(key, initial_abs_offset + tmp_buffer.size());
				}
			};

			{
				/*
					make the local subrs start immediately after the private dict.

					There's actually a similar problem here with the dictionary data; writing
					a new value for the private dict size (ie. the offset to the local subrs)
					might change the size of the private dict! so, we use the same computeSize()
					trick, and always fix the `Subrs` key to be 4-bytes. (see cff_builder.cpp)
				*/

				auto dict_builder = DictBuilder(cff->private_dict);
				auto dict_size = dict_builder.computeSize();

				copy_kv_pair_with_abs_offset(DictKey::Private, dict_size);

				dict_builder.setInteger(DictKey::Subrs, dict_size)
							.writeInto(tmp_buffer);

				// local subrs
				auto builder = IndexTableBuilder();
				for(auto& subr : cff->local_subrs)
					builder.add(subr.charstring);

				builder.writeInto(tmp_buffer);
			}

			if(cff->charset_data.has_value())
			{
				copy_kv_pair_with_abs_offset(DictKey::charset);
				tmp_buffer.append(*cff->charset_data);
			}

			if(cff->encoding_data.has_value())
			{
				copy_kv_pair_with_abs_offset(DictKey::Encoding);
				tmp_buffer.append(*cff->encoding_data);
			}

			if(cff->fdarray_table.has_value())
			{
				copy_kv_pair_with_abs_offset(DictKey::FDArray);
				IndexTableBuilder().add(*cff->fdarray_table).writeInto(tmp_buffer);
			}

			if(cff->fdselect_data.has_value())
			{
				copy_kv_pair_with_abs_offset(DictKey::FDSelect);
				tmp_buffer.append(*cff->fdselect_data);
			}

			{
				copy_kv_pair_with_abs_offset(DictKey::CharStrings);
				IndexTableBuilder().add(cff->charstrings_table).writeInto(tmp_buffer);
			}

			// finally, we write the top dict (with the new offsets) to the main buffer,
			// followed by the tmp buffer.
			IndexTableBuilder()
				.add(top_dict.serialise().span())
				.writeInto(buffer);

			buffer.append(string_table.span());
			buffer.append(global_subrs_table.span());
			buffer.append(tmp_buffer.span());
		}

#if 0
		auto f = fopen("kekw.cff", "wb");
		fwrite(buffer.data(), 1, buffer.size(), f);
		fclose(f);
#endif

		return buffer;
	}
}
