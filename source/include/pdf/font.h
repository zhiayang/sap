// font.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <zst.h>

#include "pool.h"

namespace pdf
{
	struct Document;
	struct Dictionary;

	struct Font
	{
		Dictionary* serialise(Document* doc) const;

		static Font* fromBuiltin(zst::str_view name);


		static constexpr int FONT_TYPE1 = 1;

		static constexpr int ENCODING_WIN_ANSI = 1;

	private:
		Font();

		int font_type;
		int encoding_kind;
		Dictionary* dict;

		// pool needs to be a friend because it needs the constructor
		template <typename> friend struct util::Pool;
	};
}
