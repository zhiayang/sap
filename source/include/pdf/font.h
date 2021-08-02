// font.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

namespace pdf
{
	struct Document;
	struct Dictionary;

	struct Font
	{
		Dictionary* serialise(Document* doc) const;



		int font_type;




		static constexpr int FONT_TYPE1 = 1;
	};
}
