// text.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "pdf/text.h"
#include "pdf/page.h"
#include "pdf/misc.h"

namespace pdf
{
	std::string Text::serialise(const Page* page) const
	{
		return zpr::sprint("BT\n"
				"/{} {} Tf\n"
				"{} Td\n"
				"[{}] TJ\n"
				"ET\n",
			page->getNameForFont(this->font), this->font_height, this->position,
			encodeStringLiteral(this->text)
		);
	}
}
