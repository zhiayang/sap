// page.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "pdf/page.h"
#include "pdf/object.h"

namespace pdf
{
	static const auto a4paper = Array::create(Integer::create(0), Integer::create(0), Decimal::create(595.276), Decimal::create(841.89));

	Dictionary* Page::serialise(Document* doc) const
	{
		return Dictionary::createIndirect(doc, names::Page, {
			{ names::Resources, Dictionary::create({ }) },
			{ names::MediaBox, a4paper }
		});
	}
}
