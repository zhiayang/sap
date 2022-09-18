// serialise.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>

#include "pdf/font.h"
#include "pdf/misc.h"
#include "pdf/object.h"
#include "pdf/document.h"
#include "pdf/win_ansi_encoding.h"

#include "font/font.h"

namespace pdf
{
	Font::Font()
	{
		this->font_dictionary = Dictionary::create(names::Font, {});
	}

	Dictionary* Font::serialise(Document* doc) const
	{
		if(!this->font_dictionary->is_indirect)
			this->font_dictionary->makeIndirect(doc);

		// we need to write out the widths.
		if(this->source_file && this->glyph_widths_array)
		{
			std::vector<std::pair<GlyphId, double>> widths {};
			for(auto& gid : m_used_glyphs)
			{
				auto width = this->getMetricsForGlyph(gid).horz_advance;
				widths.emplace_back(gid, this->scaleMetricForPDFTextSpace(width).value());
			}

			std::sort(widths.begin(), widths.end(), [](const auto& a, const auto& b) -> bool {
				return a.first < b.first;
			});

			std::vector<std::pair<Integer*, std::vector<Object*>>> widths2;
			for(size_t i = 0; i < widths.size(); i++)
			{
				if(widths2.empty())
				{
				foo:
					widths2.emplace_back(Integer::create(static_cast<uint32_t>(widths[i].first)),
						std::vector<Object*> { Integer::create(widths[i].second) });
				}
				else
				{
					auto& prev = widths2.back();
					if(prev.first->value + prev.second.size() == static_cast<uint32_t>(widths[i].first))
						prev.second.push_back(Integer::create(static_cast<uint32_t>(widths[i].second)));
					else
						goto foo;
				}
			}

			for(auto& [gid, ws] : widths2)
			{
				this->glyph_widths_array->values.push_back(gid);
				this->glyph_widths_array->values.push_back(Array::create(std::move(ws)));
			}
		}

		// finally, make a font subset based on the glyphs that we use.
		if(this->source_file && this->embedded_contents)
		{
			writeFontSubset(this->source_file, this->pdf_font_name, this->embedded_contents, m_used_glyphs);

			// write the cmap we'll use for /ToUnicode.
			this->writeUnicodeCMap(doc);

			// and the cidset
			this->writeCIDSet(doc);
		}

		return this->font_dictionary;
	}

	Font* Font::fromFontFile(Document* doc, font::FontFile* font_file)
	{
		auto ret = util::make<Font>();
		ret->source_file = font_file;

		/*
		    this is the general structure for composite fonts (which we always create, for now):

		    /Font: /Subtype Type0
		        /Encoding: something
		        /BaseFont: the postscript name
		        /DescendantFonts: [ bunch of CIDFonts ]
		        /ToUnicode: unicode mapping

		    /Font: /Subtype CIDFontType2/CIDFontType0 (depending on truetype or CFF)
		        /BaseFont: the postscript name
		        /W: a table of widths
		        /FontDescriptor: the font descriptor
		        /CIDSystemInfo: some useless (but required) stuff

		    /FontDescriptor:
		        /bunch of metrics;
		        /FontFile0/1/2/3: the stream containing the actual file
		        /CIDSet: if we're doing a subset (which we aren't for now)
		*/
		ret->pdf_font_name = font::generateSubsetName(font_file);
		auto basefont_name = Name::create(ret->pdf_font_name);

		// start with making the CIDFontType2/0 entry.
		auto cidfont_dict = Dictionary::createIndirect(doc, names::Font, {});

		cidfont_dict->add(names::BaseFont, basefont_name);
		cidfont_dict->add(names::CIDSystemInfo, Dictionary::create({
													{ names::Registry, String::create("Adobe") },
													{ names::Ordering, String::create("Identity") },
													{ names::Supplement, Integer::create(0) },
												}));

		bool truetype_outlines = (font_file->outline_type == font::FontFile::OUTLINES_TRUETYPE);

		ret->glyph_widths_array = Array::createIndirect(doc, {});
		cidfont_dict->add(names::W, IndirectRef::create(ret->glyph_widths_array));

		if(truetype_outlines)
		{
			cidfont_dict->add(names::CIDToGIDMap, names::Identity.ptr());
			cidfont_dict->add(names::Subtype, names::CIDFontType2.ptr());
		}
		else
		{
			cidfont_dict->add(names::Subtype, names::CIDFontType0.ptr());
		}

		/*
		    TODO:
		    for now we are outputting glyph IDs to the pdf, which is pretty dumb. in the future
		    we probably want to either:

		    (a) find a way to make a CMap that reads utf-8 from the text stream and converts that to GIDs
		    (b) use some font-specific weirdness to directly map utf-16 (or rather ucs-2) to GIDs. with
		        otf-cmap format types 6, 10, 12, and 13, we can very easily make a CMap that is not
		        monstrous in size.
		*/

		auto font_bbox = Array::create({ Integer::create(font_file->metrics.xmin), Integer::create(font_file->metrics.ymin),
			Integer::create(font_file->metrics.xmax), Integer::create(font_file->metrics.ymax) });

		int cap_height = 0;
		if(font_file->metrics.cap_height != 0)
		{
			cap_height = font_file->metrics.cap_height;
		}
		else
		{
			// basically look for the glyph for 'E' if it exists, then
			// calculate the capheight from that.

			// TODO: i don't feel like doing this now
			cap_height = 700;
			zpr::println("your dumb font doesn't tell me cap_height, assuming 700");
		}

		// truetype fonts don't contain stemv.
		static constexpr int STEMV_CONSTANT = 69;

		// TODO: use the FontFile to determine what the flags should be. no idea how important this is
		// for the pdf to display properly but it's probably entirely inconsequential.

		// we need a CIDSet for subset fonts
		ret->cidset = Stream::create(doc, {});

		// TODO: scale the metrics correctly!
		auto font_desc = Dictionary::createIndirect(doc, names::FontDescriptor,
			{ { names::FontName, basefont_name }, { names::Flags, Integer::create(4) }, { names::FontBBox, font_bbox },
				{ names::ItalicAngle, Integer::create(font_file->metrics.italic_angle) },
				{ names::Ascent, Integer::create(font_file->metrics.hhea_ascent) },
				{ names::Descent, Integer::create(font_file->metrics.hhea_descent) },
				{ names::CapHeight, Integer::create(cap_height) }, { names::XHeight, Integer::create(69) },
				{ names::StemV, Integer::create(STEMV_CONSTANT) }, { names::CIDSet, IndirectRef::create(ret->cidset) } });

		cidfont_dict->add(names::FontDescriptor, IndirectRef::create(font_desc));

		ret->embedded_contents = Stream::create(doc, {});
		ret->embedded_contents->setCompressed(true);

		if(truetype_outlines)
		{
			font_desc->add(names::FontFile2, IndirectRef::create(ret->embedded_contents));
			ret->font_type = FONT_TRUETYPE_CID;
		}
		else
		{
			// the spec is very very poorly worded on this, BUT from what I can gather, for CFF fonts we can
			// just always use FontFile3 and CIDFontType0C.

			ret->embedded_contents->dict->add(names::Subtype, names::CIDFontType0C.ptr());

			font_desc->add(names::FontFile3, IndirectRef::create(ret->embedded_contents));
			ret->font_type = FONT_CFF_CID;
		}

		// make a cmap to use for ToUnicode
		ret->unicode_cmap = Stream::create(doc, {});
		ret->unicode_cmap->setCompressed(true);

		// finally, construct the top-level Type0 font.
		auto type0 = ret->font_dictionary;
		type0->add(names::Subtype, names::Type0.ptr());
		type0->add(names::BaseFont, basefont_name);

		// TODO: here's the thing about CMap. if we use Identity-H then we don't need a CMap, but it forces the text stream
		// to contain glyph IDs instead of anything sensible.
		type0->add(names::Encoding, Name::create("Identity-H"));

		// add the unicode map so text selection isn't completely broken
		type0->add(names::ToUnicode, IndirectRef::create(ret->unicode_cmap));
		type0->add(names::DescendantFonts, Array::create(IndirectRef::create(cidfont_dict)));

		ret->encoding_kind = ENCODING_CID;

		ret->font_resource_name = zpr::sprint("F{}", doc->getNextFontResourceNumber());
		return ret;
	}

	Font* Font::fromBuiltin(Document* doc, zst::str_view name)
	{
		const char* known_fonts[] = {
			"Times-Roman",
			"Times-Italic",
			"Times-Bold",
			"Times-BoldItalic",
			"Helvetica",
			"Helvetica-Oblique",
			"Helvetica-Bold",
			"Helvetica-BoldOblique",
			"Courier",
			"Courier-Oblique",
			"Courier-Bold",
			"Courier-BoldOblique",
			"Symbol",
			"ZapfDingbats",
		};

		auto it = std::find_if(std::begin(known_fonts), std::end(known_fonts), [&name](auto& n) {
			return zst::str_view(n) == name;
		});

		if(it == std::end(known_fonts))
			pdf::error("'{}' is not a PDF builtin font", name);

		auto font = util::make<Font>();
		font->font_type = FONT_TYPE1;

		auto dict = font->font_dictionary;
		dict->add(names::Subtype, names::Type1.ptr());
		dict->add(names::BaseFont, Name::create(name));
		dict->add(names::Encoding, Name::create("WinAnsiEncoding"));
		font->encoding_kind = ENCODING_WIN_ANSI;

		font->font_resource_name = zpr::sprint("F{}", doc->getNextFontResourceNumber());

		return font;
	}
}
