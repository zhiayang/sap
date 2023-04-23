// cmap.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

// i know it's bad form to keep having two font.hs and two cmap.cpps and whatever
// but they are literally called cmaps

#include "util.h"  // for codepointToSurrogatePair
#include "types.h" // for GlyphId

#include "pdf/font.h"   // for Font, File
#include "pdf/object.h" // for Stream

#include "font/font_file.h" // for CharacterMapping, FontFile

namespace pdf
{
	// there's a limit...
	static constexpr size_t MAX_CMAP_ENTRIES = 100;

	static void write_cmap_footer(Stream* stream);

	static void add_cmap_entry(Stream* cmap, //
	    zst::str_view oper,
	    size_t num_entries,
	    size_t* current_entry,
	    zst::str_view mapping)
	{
		if(*current_entry % MAX_CMAP_ENTRIES == 0)
		{
			if(*current_entry != 0)
				cmap->append(zpr::sprint("end{}char\n", oper));

			// TODO: this is not very optimal. ideally we want to make ranges whenever we can,
			// but that requires a whole bunch of extra work.
			cmap->append(zpr::sprint("{} begin{}char\n", num_entries - *current_entry, oper));
		}

		cmap->append(mapping);
		*current_entry += 1;
	}



	void PdfFont::writeUTF8CMap() const
	{
		if(m_utf8_cmap == nullptr)
			return;

		auto font_file = dynamic_cast<const font::FontFile*>(m_source.get());
		if(not font_file)
			return;

		auto cmap = m_utf8_cmap;
		cmap->clear();

		// write the header manually, since it's a little different from the ToUnicode cmap...
		{
			int supplement = 0;
			auto registry = "sap";
			auto ordering = zpr::sprint("{}Utf8", m_pdf_font_name);

			auto cmap_name = zpr::sprint("{}-{}-{}", registry, ordering, supplement);
			auto header = zpr::sprint(
			    "%!PS-Adobe-3.0 Resource-CMap\n"
			    "%%DocumentNeededResources: ProcSet (CIDInit)\n"
			    "%%IncludeResource: ProcSet (CIDInit)\n"
			    "%%BeginResource: CMap ({})\n"
			    "%%Title: ({} {} {} {})\n"
			    "%%Version: 1\n"
			    "%%EndComments\n"
			    "/CIDInit /ProcSet findresource begin\n"
			    "12 dict begin\n"
			    "begincmap\n"
			    "/CIDSystemInfo 3 dict dup begin\n"
			    "  /Registry ({}) def\n"
			    "  /Ordering ({}) def\n"
			    "  /Supplement {} def\n"
			    "end def\n"
			    "/CMapName /{} def\n"
			    "/CMapType 0 def\n"
			    "/WMode 0 def\n"
			    "/XUID [1 10 12345] def\n",
			    cmap_name,                                 //
			    cmap_name, registry, ordering, supplement, //
			    registry,                                  //
			    ordering,                                  //
			    supplement,                                //
			    cmap_name                                  //
			);

			cmap->append(header);
		}

		cmap->append(
		    "4 begincodespacerange\n"
		    "<00>       <7F>\n"
		    "<C080>     <DFBF>\n"
		    "<E08080>   <EFBFBF>\n"
		    "<F0808080> <F7BFBFBF>\n"
		    "endcodespacerange\n");

		assert(m_source != nullptr);
		auto& mapping = font_file->characterMapping().forward;

		size_t current_cmap_entry = 0;
		auto do_mapping = [cmap, &current_cmap_entry](size_t num_glyphs, GlyphId glyph, char32_t codepoint) {
			auto utf8 = unicode::utf8FromCodepoint(codepoint);
			assert(utf8.size() <= 4);

			std::string tmp = "<";
			for(char x : utf8)
				tmp += zpr::sprint("{02x}", static_cast<uint8_t>(x));

			tmp += zpr::sprint("> {}\n", static_cast<uint32_t>(glyph));
			add_cmap_entry(cmap, "cid", num_glyphs, &current_cmap_entry, tmp);
		};

		auto num_used_glyphs = font_file->usedGlyphs().size();
		for(auto& [cp, glyph] : mapping)
		{
			if(not font_file->isGlyphUsed(glyph))
				continue;

			do_mapping(num_used_glyphs, glyph, cp);
		}

		// reset.
		cmap->append("endcidchar\n");
		current_cmap_entry = 0;

		// now for the extra bois.
		if(auto num_extras = m_extra_glyph_to_private_use_mapping.size(); num_extras > 0)
		{
			for(auto [glyph, cp] : m_extra_glyph_to_private_use_mapping)
				do_mapping(num_extras, glyph, cp);

			cmap->append("endcidchar\n");
		}

		write_cmap_footer(cmap);
	}






	void PdfFont::writeUnicodeCMap() const
	{
		auto font_file = dynamic_cast<const font::FontFile*>(m_source.get());
		if(not font_file)
			return;

		auto cmap = m_tounicode_cmap;
		cmap->clear();

		{
			auto registry = "sap";
			auto ordering = zpr::sprint("{}ToUnicode", m_pdf_font_name);
			auto supplement = 0;
			auto cmap_name = zpr::sprint("{}-{}-{}", registry, ordering, supplement);

			auto header = zpr::sprint(
			    "%!PS-Adobe-3.0 Resource-CMap\n"
			    "%%DocumentNeededResources: ProcSet (CIDInit)\n"
			    "%%IncludeResource: ProcSet (CIDInit)\n"
			    "%%BeginResource: CMap ({})\n"
			    "%%Title: ({} {} {} {})\n"
			    "%%Version: 1\n"
			    "%%EndComments\n"
			    "/CIDInit /ProcSet findresource begin\n"
			    "12 dict begin\n"
			    "begincmap\n"
			    "/CIDSystemInfo <<\n"
			    "  /Registry ({})\n"
			    "  /Ordering ({})\n"
			    "  /Supplement {}\n"
			    ">> def\n"
			    "/CMapName /{} def\n"
			    "/CMapType 2 def\n",
			    cmap_name,                                 //
			    cmap_name, registry, ordering, supplement, //
			    registry,                                  //
			    ordering,                                  //
			    supplement,                                //
			    cmap_name                                  //
			);

			cmap->append(header);
		}

		cmap->append(
		    "1 begincodespacerange\n"
		    "<0000> <FFFF>\n"
		    "endcodespacerange\n");

		assert(m_source != nullptr);
		auto& mapping = font_file->characterMapping().forward;

		size_t current_cmap_entry = 0;

		auto num_used_glyphs = font_file->usedGlyphs().size();
		for(auto& [cp, glyph] : mapping)
		{
			if(not font_file->isGlyphUsed(glyph))
				continue;

			auto codepoint = static_cast<uint32_t>(cp);
			if(codepoint <= 0xFFFF)
			{
				add_cmap_entry(cmap, "bf", num_used_glyphs, &current_cmap_entry,
				    zpr::sprint("<{04x}> <{04x}>\n", glyph, codepoint));
			}
			else
			{
				auto [high, low] = unicode::codepointToSurrogatePair(cp);
				add_cmap_entry(cmap, "bf", num_used_glyphs, &current_cmap_entry,
				    zpr::sprint("<{04x}> <{04x}{04x}>\n", glyph, high, low));
			}
		}

		// reset.
		cmap->append("endbfchar\n");
		current_cmap_entry = 0;


		// now for the ligatures.
		if(auto num_entries = m_extra_unicode_mappings.size(); num_entries > 0)
		{
			for(auto& [gid, cps] : m_extra_unicode_mappings)
			{
				std::string s {};
				{
					auto app = zpr::detail::string_appender(s);
					zpr::cprint(app, "<{04x}> <", static_cast<uint32_t>(gid));

					for(auto cp : cps)
					{
						if(cp <= 0xFFFF)
						{
							zpr::cprint(app, "{04x}", static_cast<uint16_t>(cp));
						}
						else
						{
							auto [high, low] = unicode::codepointToSurrogatePair(cp);
							zpr::cprint(app, "{04x}{04x}", high, low);
						}
					}
				}
				s += ">\n";
				add_cmap_entry(cmap, "bf", num_entries, &current_cmap_entry, s);
			}
			cmap->append("endbfchar\n");
		}

		write_cmap_footer(cmap);
	}







	static void write_cmap_footer(Stream* s)
	{
		s->append(zst::str_view(
		    "endcmap\n"
		    "CMapName currentdict /CMap defineresource pop\n"
		    "end\n"
		    "end\n"
		    "%%EndResource\n"
		    "%%EOF\n"));
	}
}
