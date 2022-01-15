// page.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <set>

#include "pdf/object.h"
#include "pdf/pageobject.h"

namespace pdf
{
	struct Font;
	struct Document;

	struct Page
	{
		Dictionary* serialise(Document* doc) const;

		void useFont(const Font* font) const;
		void addObject(PageObject* obj);

	private:
		mutable std::vector<const Font*> fonts;
		std::vector<PageObject*> objects;
	};
}
