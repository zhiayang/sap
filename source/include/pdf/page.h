// page.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/units.h"      // for Size2d, Vector2_YDown, Vector2_YUp
#include "pdf/object.h"     // for Dictionary, Document
#include "pdf/pageobject.h" // for PageObject

namespace pdf
{
	struct Font;
	struct Document;

	struct Page
	{
		Page();

		Dictionary* serialise(Document* doc) const;

		void useFont(const Font* font) const;
		void addObject(PageObject* obj);

		Size2d size() const;

		Vector2_YUp convertVector2(Vector2_YDown v2) const;
		Vector2_YDown convertVector2(Vector2_YUp v2) const;

	private:
		mutable std::vector<const Font*> fonts;
		std::vector<PageObject*> objects;

		Size2d m_page_size {};
	};
}
