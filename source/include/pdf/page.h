// page.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/object.h"
#include "pdf/pageobject.h"

namespace pdf
{
	struct Font;
	struct Document;

	struct Page
	{
		Dictionary* serialise(Document* doc) const;

		void useFont(Font* font);
		void addObject(PageObject* obj);

		std::string getNameForFont(Font* font) const;

	private:
		mutable std::vector<Font*> fonts;
		std::vector<PageObject*> objects;
	};
}
