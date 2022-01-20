// subset.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"
#include "error.h"

#include "font/cff.h"
#include "font/font.h"

namespace font::cff
{
	static void write_charset_table(CFFData* cff, zst::byte_buffer& buffer, const std::map<uint32_t, GlyphMetrics>& used_glyphs)
	{
		// format 0
		buffer.append(0);

		for(auto& g : cff->glyphs)
		{
			// don't add notdef
			if(g.gid == 0)
				continue;

			if(cff->is_cidfont)
				buffer.append_bytes(util::convertBEU16(g.cid));
			else
				buffer.append_bytes(util::convertBEU16(g.gid));
		}
	}

	static void prune_unused_glyphs(CFFData* cff, const std::map<uint32_t, GlyphMetrics>& used_glyphs)
	{
		auto foo = std::remove_if(cff->glyphs.begin(), cff->glyphs.end(), [&](auto& glyph) -> bool {
			if(!cff->is_cidfont)
			{
				// for non-CID fonts, the glyph we get from sap/pdf is already the gid.
				return glyph.gid != 0 && used_glyphs.find(glyph.gid) == used_glyphs.end();
			}
			else
			{
				/*
					for CID fonts, see PDF 1.7, 9.7.4.2 Glyph Selection in CIDFonts:

					The CIDs shall be used to determine the GID value for the glyph procedure using
					the charset table in the CFF program. The GID value shall then be used to look up
					the glyph procedure using the CharStrings INDEX table.

					this seems to imply that glyph ids gotten from the PDF layer are actually CIDs to
					the CFF font; thus, we match against CIDs instead.
				*/
				return glyph.gid != 0 && used_glyphs.find(glyph.cid) == used_glyphs.end();
			}
		});
		cff->glyphs.erase(foo, cff->glyphs.end());
	}


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

		/*
			For now, this is our subsetting strategy (for CFF1 at least):

			1. we always generate CID fonts, if not we will potentially waste a lot of space
				in the various tables (charset and string)

			2. use a format3 FDSelect, and just direct all glyphs to the same Font DICT.
				the font dict doesn't contain any information except the FontName (the subset name)
				and the private dict.

				(this is the same Private DICT as what came from the original Top DICT)

			3. charset... from what I can understand, when using a CIDFont (which we always do) in the PDF,
				the text in a PDF will refer to a CID. For truetype fonts, CID=GID, so all is good.

				For CFF fonts, we need to use the `charset` table to translate CID to GID. However, it
				appears as if the charset table translates GIDs to CIDs instead. Maybe the PDF renderer
				is forced to build an internal reverse mapping for this.
		*/

		// first, figure out which glyphs we don't use.
		prune_unused_glyphs(cff, used_glyphs);


		// write the header
		buffer.append(1);   // major
		buffer.append(0);   // minor
		buffer.append(4);   // hdrSize
		buffer.append(4);   // offSize (for now, always 4. I don't even know what this field is used for)

		// Name INDEX (there is only 1)
		IndexTableBuilder()
			.add(subset_name.bytes())
			.writeInto(buffer);

		auto subset_name_sid = cff->get_or_add_string(subset_name);

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
		top_dict.set(DictKey::ROS, {
			Operand().string_id(cff->get_or_add_string("Adobe")),   // registry
			Operand().string_id(cff->get_or_add_string("Identity")),// ordering
			Operand().integer(0)                                    // supplement
		});

		// pre-set these to reserve space for them
		top_dict.setInteger(DictKey::FDArray, 0);
		top_dict.setInteger(DictKey::FDSelect, 0);
		top_dict.setInteger(DictKey::CIDCount, cff->glyphs.size());

		// Encoding can't be present for CID fonts, and Private will come from
		// the FDArray/FDSelect Font DICT instead of the Top one.
		top_dict.erase(DictKey::Encoding);
		top_dict.erase(DictKey::Private);

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

		// String INDEX
		zst::byte_buffer string_table {};
		{
			auto builder = IndexTableBuilder();
			for(auto& str : cff->string_ids)
				builder.add(zst::str_view(str).cast<uint8_t>());

			builder.writeInto(string_table);
		}


		// Global Subrs INDEX
		zst::byte_buffer global_subrs_table {};
		{
			auto builder = IndexTableBuilder();
			for(auto& subr : cff->global_subrs)
				builder.add(subr.charstring);

			builder.writeInto(global_subrs_table);
		}


		// all the other stuff that comes after the Top DICT
		{
			zst::byte_buffer tmp_buffer {};
			const auto initial_abs_offset = buffer.size()
				+ top_dict_size
				+ string_table.size()
				+ global_subrs_table.size();

			// call this *BEFORE* writing the data!!!
			auto copy_kv_pair_with_abs_offset = [&](DictKey key, size_t size = 0) {
				// Private needs special treatment (since it needs both a size and offset)
				if(key == DictKey::Private)
					top_dict.setIntegerPair(key, size, initial_abs_offset + tmp_buffer.size());
				else
					top_dict.setInteger(key, initial_abs_offset + tmp_buffer.size());
			};


			{
				auto private_dict_builder = DictBuilder(cff->private_dict);
				auto private_dict_size = private_dict_builder.computeSize();
				auto private_dict_ofs = initial_abs_offset + tmp_buffer.size();

				private_dict_builder.setInteger(DictKey::Subrs, private_dict_size)
									.writeInto(tmp_buffer);

				/*
					make the local subrs start immediately after the private dict.

					There's actually a similar problem here with the dictionary data; writing
					a new value for the private dict size (ie. the offset to the local subrs)
					might change the size of the private dict! so, we use the same computeSize()
					trick, and always fix the `Subrs` key to be 4-bytes. (see cff_builder.cpp)
				*/
				{
					auto local_subrs_builder = IndexTableBuilder();
					for(auto& subr : cff->local_subrs)
						local_subrs_builder.add(subr.charstring);

					local_subrs_builder.writeInto(tmp_buffer);
				}

				copy_kv_pair_with_abs_offset(DictKey::FDArray);
				auto fdarray_builder = DictBuilder()
					.setInteger(DictKey::FontName, subset_name_sid)
					.setIntegerPair(DictKey::Private, private_dict_size, private_dict_ofs);

				IndexTableBuilder()
					.add(fdarray_builder.serialise().span())
					.writeInto(tmp_buffer);

				// write the fdselect.
				copy_kv_pair_with_abs_offset(DictKey::FDSelect);
				{
					tmp_buffer.append(3);
					tmp_buffer.append_bytes(util::convertBEU16(1));
					tmp_buffer.append_bytes(util::convertBEU16(0));
					tmp_buffer.append(0);
					tmp_buffer.append_bytes(util::convertBEU16(cff->glyphs.size()));
				}
			}

			{
				copy_kv_pair_with_abs_offset(DictKey::charset);
				write_charset_table(cff, tmp_buffer, used_glyphs);
			}

			{
				copy_kv_pair_with_abs_offset(DictKey::CharStrings);

				auto builder = IndexTableBuilder();
				for(auto& glyph : cff->glyphs)
					builder.add(glyph.charstring);

				builder.writeInto(tmp_buffer);
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

#if 1
		auto f = fopen("kekw.cff", "wb");
		fwrite(buffer.data(), 1, buffer.size(), f);
		fclose(f);
#endif

		return buffer;
	}
}
