// subset.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>

#include "util.h"
#include "error.h"

#include "font/cff.h"
#include "font/font.h"

namespace font::cff
{
	static void write_charset_table(CFFData* cff, zst::byte_buffer& buffer, const std::unordered_set<GlyphId>& used_glyphs)
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

	static void perform_glyph_pruning(CFFData* cff, const std::unordered_set<GlyphId>& used_glyphs)
	{
		std::set<uint8_t> used_font_dicts {};

		auto foo = std::remove_if(cff->glyphs.begin(), cff->glyphs.end(), [&](auto& glyph) -> bool {
			if(!cff->is_cidfont)
			{
				// for non-CID fonts, the glyph we get from sap/pdf is already the gid.
				return glyph.gid != 0 && used_glyphs.find(GlyphId { glyph.gid }) == used_glyphs.end();
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
				bool unused = glyph.gid != 0 && used_glyphs.find(GlyphId { glyph.cid }) == used_glyphs.end();

				// also, mark the fontdict as used if the glyph was used.
				if(!unused)
					used_font_dicts.insert(glyph.font_dict_idx);

				return unused;
			}
		});

		cff->glyphs.erase(foo, cff->glyphs.end());

		// if there were any unused font dicts, we can prune them
		if(used_font_dicts.size() != cff->font_dicts.size())
		{
			std::map<uint8_t, uint8_t> fd_mapping {};
			std::vector<FontDict> new_font_dicts {};

			// use the `used_font_dicts` as a "not visited" array
			for(auto& glyph : cff->glyphs)
			{
				if(fd_mapping.find(glyph.font_dict_idx) == fd_mapping.end())
				{
					fd_mapping[glyph.font_dict_idx] = new_font_dicts.size();
					new_font_dicts.push_back(std::move(cff->font_dicts[glyph.font_dict_idx]));
				}

				glyph.font_dict_idx = fd_mapping[glyph.font_dict_idx];
			}

			cff->font_dicts = std::move(new_font_dicts);
		}


		// finally, interpret the charstrings of all used glyphs, and mark used subrs for elimination.
		for(auto& glyph : cff->glyphs)
		{
			interpretCharStringAndMarkSubrs(glyph.charstring, cff->global_subrs,
				cff->font_dicts[glyph.font_dict_idx].local_subrs);
		}
	}


	CFFSubset createCFFSubset(FontFile* font, zst::str_view subset_name, const std::unordered_set<GlyphId>& used_glyphs)
	{
		auto cff = font->cff_data;
		assert(cff != nullptr);

		zst::byte_buffer buffer {};

		// we only support subsetting CFF1 data for now.
		if(cff->cff2)
		{
			buffer.append(cff->bytes);

			CFFSubset ret {};
			ret.cff = std::move(buffer);

			auto orig_cmap_table = font->tables[Tag("cmap")];
			ret.cmap.append(
				zst::byte_span(font->file_bytes, font->file_size).drop(orig_cmap_table.offset).take(orig_cmap_table.length));

			return ret;
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

		    4. for subrs, we interpret all the used glyphs and mark both global and local subrs that are used.
		        for unused subrs, we *still emit them* to the file, but with a 0-length charstring.

		        this allows us to not need to modify the charstring data for glyphs and re-number the subrs.
		        once the CFF is compressed, any decent compression algorithm should be able to reduce the
		        repeated values.
		*/

		// first, figure out which glyphs we don't use.
		perform_glyph_pruning(cff, used_glyphs);


		// write the header
		buffer.append(1); // major
		buffer.append(0); // minor
		buffer.append(4); // hdrSize
		buffer.append(4); // offSize (for now, always 4. I don't even know what this field is used for)

		// Name INDEX (there is only 1)
		IndexTableBuilder().add(subset_name.bytes()).writeInto(buffer);

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
									   Operand().string_id(cff->get_or_add_string("Adobe")),    // registry
									   Operand().string_id(cff->get_or_add_string("Identity")), // ordering
									   Operand().integer(0)                                     // supplement
								   });

		// pre-set these to reserve space for them
		top_dict.setInteger(DictKey::FDArray, 0);
		top_dict.setInteger(DictKey::FDSelect, 0);
		top_dict.setStringId(DictKey::FontName, subset_name_sid);
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
			if(top_dict_size < 255)
				off_size = 1;
			else if(top_dict_size < 65535)
				off_size = 2;
			else
				off_size = 4;

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
			{
				if(subr.used)
					builder.add(subr.charstring);
				else
					builder.add(zst::byte_span {});
			}

			builder.writeInto(global_subrs_table);
		}


		// all the other stuff that comes after the Top DICT
		{
			zst::byte_buffer tmp_buffer {};
			const auto initial_abs_offset = buffer.size() + top_dict_size + string_table.size() + global_subrs_table.size();

			const auto current_abs_ofs = [&]() {
				return initial_abs_offset + tmp_buffer.size();
			};

			// call this *BEFORE* writing the data!!!
			auto copy_kv_pair_with_abs_offset = [&](DictKey key, size_t size = 0) {
				// Private needs special treatment (since it needs both a size and offset)
				if(key == DictKey::Private)
					top_dict.setIntegerPair(key, size, current_abs_ofs());
				else
					top_dict.setInteger(key, current_abs_ofs());
			};

			// returns the absolute offset to the private dict
			auto serialise_fontdict = [&](FontDict& fd) -> std::pair<size_t, size_t> {
				auto priv_builder = DictBuilder(fd.private_dict);
				auto priv_size = priv_builder.computeSize();
				auto priv_ofs = current_abs_ofs();

				// make the local subrs start immediately after the private dict.
				if(fd.local_subrs.size() > 0)
					priv_builder.setInteger(DictKey::Subrs, priv_size);

				priv_builder.writeInto(tmp_buffer);

				if(fd.local_subrs.size() > 0)
				{
					auto builder = IndexTableBuilder();
					for(auto& subr : fd.local_subrs)
					{
						if(subr.used)
							builder.add(subr.charstring);
						else
							builder.add(zst::byte_span {});
					}

					builder.writeInto(tmp_buffer);
				}

				return { priv_size, priv_ofs };
			};


			std::vector<std::pair<size_t, size_t>> private_dicts {};

			// if the original font was not a CIDFont, then we just create new ones, easy.
			if(!cff->is_cidfont)
			{
				private_dicts.push_back(serialise_fontdict(cff->font_dicts[0]));
			}
			else
			{
				for(auto& fd : cff->font_dicts)
					private_dicts.push_back(serialise_fontdict(fd));
			}

			{
				copy_kv_pair_with_abs_offset(DictKey::FDArray);

				auto fdarray_builder = IndexTableBuilder();
				for(auto& [priv_size, priv_ofs] : private_dicts)
				{
					auto builder = DictBuilder()
					                   .setStringId(DictKey::FontName, subset_name_sid)
					                   .setIntegerPair(DictKey::Private, priv_size, priv_ofs);

					fdarray_builder.add(builder.serialise().span());
				}

				fdarray_builder.writeInto(tmp_buffer);


				// finally, the fdselect...
				copy_kv_pair_with_abs_offset(DictKey::FDSelect);

				if(!cff->is_cidfont)
				{
					// for non-cid fonts, we just use the same FD for all glyphs, easy.
					tmp_buffer.append(3);
					tmp_buffer.append_bytes(util::convertBEU16(1));
					tmp_buffer.append_bytes(util::convertBEU16(0));
					tmp_buffer.append(0);
					tmp_buffer.append_bytes(util::convertBEU16(cff->glyphs.size() + 1));
				}
				else
				{
					// for CID fonts, we just use whatever the font tells us to use.
					// format 0, because idgaf
					tmp_buffer.append(0);
					for(auto& glyph : cff->glyphs)
						tmp_buffer.append(glyph.font_dict_idx);
				}
			}


			copy_kv_pair_with_abs_offset(DictKey::charset);
			write_charset_table(cff, tmp_buffer, used_glyphs);


			{
				copy_kv_pair_with_abs_offset(DictKey::CharStrings);

				auto builder = IndexTableBuilder();
				for(auto& glyph : cff->glyphs)
					builder.add(glyph.charstring);

				builder.writeInto(tmp_buffer);
			}


			// finally, we write the top dict (with the new offsets) to the main buffer,
			// followed by the tmp buffer.
			IndexTableBuilder().add(top_dict.serialise().span()).writeInto(buffer);

			buffer.append(string_table.span());
			buffer.append(global_subrs_table.span());
			buffer.append(tmp_buffer.span());
		}


#if 0
		auto f = fopen("kekw.cff", "wb");
		fwrite(buffer.data(), 1, buffer.size(), f);
		fclose(f);
#endif

		CFFSubset ret {};
		ret.cff = std::move(buffer);
		ret.cmap = createCMapForCFFSubset(font);

		return ret;
	}
}
