// font.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <algorithm>

#include "otf/otf.h"

#include "pdf/font.h"
#include "pdf/misc.h"
#include "pdf/object.h"

namespace pdf
{
	Font::Font()
	{
		this->font_dictionary = Dictionary::create(names::Font, { });
	}

	Dictionary* Font::serialise(Document* doc) const
	{
		if(!this->font_dictionary->is_indirect)
			this->font_dictionary->makeIndirect(doc);

		return this->font_dictionary;
	}










	Font* Font::fromFontFile(Document* doc, otf::OTFont* font)
	{
		auto ret = util::make<Font>();
		ret->source_file = font;

		if(font->font_type != otf::OTFont::TYPE_TRUETYPE && font->font_type != otf::OTFont::TYPE_CFF)
			pdf::error("unsupported font type");

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
		auto basefont_name = Name::create(font->postscript_name);

		// start with making the CIDFontType2/0 entry.
		auto cidfont_dict = Dictionary::createIndirect(doc, names::Font, { });

		cidfont_dict->add(names::BaseFont, basefont_name);
		cidfont_dict->add(names::CIDSystemInfo, Dictionary::create({
			{ names::Registry, names::Sap.ptr() },
			{ names::Ordering, names::Identity.ptr() },
			{ names::Supplement, Integer::create(0) },
		}));

		if(font->font_type == otf::OTFont::TYPE_TRUETYPE)
		{
			cidfont_dict->add(names::CIDToGIDMap, names::Identity.ptr());
			cidfont_dict->add(names::Subtype, names::CIDFontType2.ptr());
		}
		else if(font->font_type == otf::OTFont::TYPE_CFF)
		{
			cidfont_dict->add(names::Subtype, names::CIDFontType0.ptr());
		}
		else
		{
			pdf::error("unsupported font type");
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

		// see note in otf/parser.cpp about the scaling with units_per_em
		auto units_per_em_scale = 1000.0 / font->metrics.units_per_em;


		auto font_bbox = Array::create({
			Integer::create(font->metrics.xmin),
			Integer::create(font->metrics.ymin),
			Integer::create(font->metrics.xmax),
			Integer::create(font->metrics.ymax)
		});

		int cap_height = 0;
		if(font->metrics.cap_height != 0)
		{
			cap_height = font->metrics.cap_height;
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

		// TODO: use the OTFont to determine what the flags should be. no idea how important this is
		// for the pdf to display properly but it's probably entirely inconsequential.
		auto font_desc = Dictionary::createIndirect(doc, names::FontDescriptor, {
			{ names::FontName, basefont_name },
			{ names::Flags, Integer::create(4) },
			{ names::FontBBox, font_bbox },
			{ names::ItalicAngle, Decimal::create(font->metrics.italic_angle) },
			{ names::Ascent, Integer::create(font->metrics.ascent * units_per_em_scale) },
			{ names::Descent, Integer::create(font->metrics.descent * units_per_em_scale) },
			{ names::CapHeight, Integer::create(cap_height) },
			{ names::StemV, Integer::create(STEMV_CONSTANT) }
		});

		cidfont_dict->add(names::FontDescriptor, IndirectRef::create(font_desc));

		/*
			TODO: we want to add the width table (/W) to the cidfont_dict as well. if we don't then the default
			width is used for all glyphs, which is probably ugly af.
		*/

		/*
			TODO: for now we are dumping the entire file into the pdf; we probably want some kind of subsetting functionality,
			which requires improving the robustness of the OTF parser (and handling its glyf [or the CFF equiv.) table.
		*/
		if(font->font_type == otf::OTFont::TYPE_TRUETYPE)
		{
			// TODO: this makes a copy of the file (since Stream wants to own the memory also),
			// which is not ideal.
			auto file_contents = Stream::create(doc, { });
			file_contents->append(font->file_bytes, font->file_size);

			font_desc->add(names::FontFile2, IndirectRef::create(file_contents));
			ret->font_type = FONT_TRUETYPE_CID;
		}
		else if(font->font_type == otf::OTFont::TYPE_CFF)
		{
			// this is too complicated for me.
			assert(!"TODO: CFF fonts not implemented");
			ret->font_type = FONT_CFF_CID;
		}


		// finally, construct the top-level Type0 font.
		auto type0 = ret->font_dictionary;
		type0->add(names::Subtype, names::Type0.ptr());
		type0->add(names::BaseFont, basefont_name);

		// TODO: here's the thing about CMap. if we use Identity-H then we don't need a CMap, but it forces the text stream
		// to contain glyph IDs instead of anything sensible.
		type0->add(names::Encoding, Name::create("Identity-H"));

		type0->add(names::DescendantFonts, Array::create(IndirectRef::create(cidfont_dict)));

		return ret;
	}

	Font* Font::fromBuiltin(zst::str_view name)
	{
		const char* known_fonts[] = {
			"Times-Roman", "Times-Italic", "Times-Bold", "Times-BoldItalic",
			"Helvetica", "Helvetica-Oblique", "Helvetica-Bold", "Helvetica-BoldOblique",
			"Courier", "Courier-Oblique", "Courier-Bold", "Courier-BoldOblique",
			"Symbol", "ZapfDingbats",
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

		return font;
	}
}
