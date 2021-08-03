// font.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <zst.h>

#include "pool.h"

namespace otf
{
	struct OTFont;
}

namespace pdf
{
	struct Stream;
	struct Document;
	struct Dictionary;

	struct Font
	{
		Dictionary* serialise(Document* doc) const;

		static Font* fromBuiltin(zst::str_view name);
		static Font* fromFontFile(Document* doc, otf::OTFont* font);

		static constexpr int FONT_TYPE1         = 1;
		static constexpr int FONT_TRUETYPE      = 2;
		static constexpr int FONT_CFF_CID       = 3;
		static constexpr int FONT_TRUETYPE_CID  = 4;

		static constexpr int ENCODING_WIN_ANSI = 1;

	private:
		Font();

		int font_type = 0;
		int encoding_kind = 0;
		Dictionary* font_dictionary = 0;

		// only used for embedded fonts
		Dictionary* font_descriptor = 0;

		// pool needs to be a friend because it needs the constructor
		template <typename> friend struct util::Pool;
	};
}
