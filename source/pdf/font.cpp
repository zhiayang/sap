// font.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "pdf/font.h"
#include "pdf/misc.h"
#include "pdf/object.h"

namespace pdf
{
	Font::Font()
	{
		this->dict = Dictionary::create(names::Font, { });
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

		auto dict = font->dict;
		dict->add(names::Subtype, names::Type1.ptr());
		dict->add(names::BaseFont, Name::create(name));
		dict->add(names::Encoding, Name::create("WinAnsiEncoding"));
		font->encoding_kind = ENCODING_WIN_ANSI;

		return font;
	}

	Dictionary* Font::serialise(Document* doc) const
	{
		if(!this->dict->is_indirect)
			this->dict->makeIndirect(doc);

		return this->dict;
	}
}
