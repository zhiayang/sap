// font.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <map>
#include <zst.h>

#include "pool.h"
#include "font/font.h"

namespace pdf
{
	struct Array;
	struct Stream;
	struct Document;
	struct Dictionary;

	struct Font
	{
		using KerningPair = std::pair<font::GlyphAdjustment, font::GlyphAdjustment>;

		Dictionary* serialise(Document* doc) const;

		uint32_t getGlyphIdFromCodepoint(uint32_t codepoint) const;
		std::optional<KerningPair> getKerningForGlyphs(uint32_t glyph1, uint32_t glyph2) const;

		int font_type = 0;
		int encoding_kind = 0;

		static Font* fromBuiltin(zst::str_view name);
		static Font* fromFontFile(Document* doc, font::FontFile* font);

		static constexpr int FONT_TYPE1         = 1;
		static constexpr int FONT_TRUETYPE      = 2;
		static constexpr int FONT_CFF_CID       = 3;
		static constexpr int FONT_TRUETYPE_CID  = 4;

		static constexpr int ENCODING_WIN_ANSI  = 1;
		static constexpr int ENCODING_CID       = 2;



	private:
		Font();
		Dictionary* font_dictionary = 0;

		mutable std::map<uint32_t, uint32_t> cmap_cache { };
		mutable std::map<uint32_t, font::GlyphMetrics> glyph_metrics { };
		mutable std::map<std::pair<uint32_t, uint32_t>, KerningPair> kerning_pairs { };

		// only used for embedded fonts
		font::FontFile* source_file = 0;
		Array* glyph_widths_array = 0;

		// pool needs to be a friend because it needs the constructor
		template <typename> friend struct util::Pool;
	};
}
