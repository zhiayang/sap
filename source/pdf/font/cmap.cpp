// cmap.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

// i know it's bad form to keep having two font.hs and two cmap.cpps and whatever
// but they are literally called cmaps

#include "util.h"
#include "pdf/font.h"
#include "pdf/object.h"

namespace pdf
{
	static void write_cmap_header(Stream* stream, zst::str_view font_name);
	static void write_cmap_footer(Stream* stream);

	void Font::writeUnicodeCMap(Document* doc) const
	{
		auto cmap = this->unicode_cmap;
		write_cmap_header(cmap, this->pdf_font_name);

		cmap->append("1 begincodespacerange\n"
					 "<0000> <FFFF>\n"
					 "endcodespacerange\n");

		assert(this->source_file != nullptr);
		auto& mapping = this->source_file->character_mapping.forward;

		// TODO: this is not very optimal. ideally we want to make ranges whenever we can,
		// but that requires a whole bunch of extra work.
		cmap->append(zpr::sprint("{} beginbfchar\n", mapping.size()));

		for(auto& [cp, glyph] : mapping)
		{
			if(m_used_glyphs.find(glyph) == m_used_glyphs.end())
				continue;

			auto codepoint = static_cast<uint32_t>(cp);
			if(codepoint <= 0xFFFF)
			{
				cmap->append(zpr::sprint("<{04x}> <{04x}>\n", glyph, codepoint));
			}
			else
			{
				auto [high, low] = unicode::codepointToSurrogatePair(cp);
				cmap->append(zpr::sprint("<{04x}> <{04x}{04x}>\n", glyph, high, low));
			}
		}

		cmap->append("endbfchar\n");


		// now for the ligatures.
		cmap->append(zpr::sprint("{} beginbfchar\n", m_extra_unicode_mappings.size()));
		for(auto& [gid, cps] : m_extra_unicode_mappings)
		{
			cmap->append(zpr::sprint("<{04x}> <", gid));
			for(auto cp : cps)
			{
				if(cp <= 0xFFFF_codepoint)
				{
					cmap->append(zpr::sprint("{04x}", cp));
				}
				else
				{
					auto [high, low] = unicode::codepointToSurrogatePair(cp);
					cmap->append(zpr::sprint("{04x}{04x}", high, low));
				}
			}

			cmap->append(">\n");
		}
		cmap->append("endbfchar\n");


		write_cmap_footer(cmap);
	}








	static void write_cmap_header(Stream* s, zst::str_view font_name)
	{
		auto cmap_name = zpr::sprint("Sap-{}-0", font_name);
		auto ordering = "UnicodeOrdering";
		auto registry = "Sap";
		auto supplement = 0;

		auto header = zpr::sprint("%!PS-Adobe-3.0 Resource-CMap\n"
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

			cmap_name, cmap_name, registry, ordering, supplement, registry, ordering, supplement, cmap_name);

		s->append(header);
	}

	static void write_cmap_footer(Stream* s)
	{
		s->append(zst::str_view("endcmap\n"
								"CMapName currentdict /CMap defineresource pop\n"
								"end\n"
								"end\n"
								"%%EndResource"
								"%%EOF"));
	}
}
