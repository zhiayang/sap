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

		if(this->font_descriptor != nullptr && !this->font_descriptor->is_indirect)
			this->font_dictionary->makeIndirect(doc);

		return this->font_dictionary;
	}










	Font* Font::fromFontFile(Document* doc, otf::OTFont* font)
	{
		auto ret = util::make<Font>();
		auto dict = ret->font_dictionary;

		if(font->font_type != otf::OTFont::TYPE_TRUETYPE && font->font_type != otf::OTFont::TYPE_CFF)
			pdf::error("unsupported font type");

		dict->add(names::BaseFont, Name::create(font->postscript_name));
		dict->add(names::CIDSystemInfo, Dictionary::create({
			{ names::Registry, names::Sap.ptr() },
			{ names::Ordering, names::Identity.ptr() },
			{ names::Supplement, Integer::create(0) },
		}));

		// our strategy for now is always to create a Type0 CID font,
		// instead of doing some weird special-casing.
		if(font->font_type == otf::OTFont::TYPE_TRUETYPE)
		{
			dict->add(names::Subtype, names::CIDFontType2.ptr());
			ret->font_type = FONT_TRUETYPE_CID;
		}
		else if(font->font_type == otf::OTFont::TYPE_CFF)
		{
			dict->add(names::Subtype, names::CIDFontType0.ptr());
			ret->font_type = FONT_CFF_CID;
		}

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
