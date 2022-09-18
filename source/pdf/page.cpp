// page.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>

#include "pdf/page.h"
#include "pdf/font.h"
#include "pdf/misc.h"
#include "pdf/object.h"
#include "pdf/pageobject.h"

namespace pdf
{
	// TODO: support custom paper sizes
	static const auto a4paper =
		Array::create(Integer::create(0), Integer::create(0), Decimal::create(595.276), Decimal::create(841.89));

	Page::Page()
	{
		m_page_size = pdf::Size2d(595.276, 841.89);
	}

	Size2d Page::size() const
	{
		return m_page_size;
	}

	Vector2_YUp Page::convertVector2(Vector2_YDown v2) const
	{
		return Vector2_YUp(v2.x().value(), (m_page_size.y() - v2.y().into<dim::units::pdf_typographic_unit>({})).value());
	}

	Vector2_YDown Page::convertVector2(Vector2_YUp v2) const
	{
		return Vector2_YDown(v2.x().value(), (m_page_size.y() - v2.y()).value());
	}


	Dictionary* Page::serialise(Document* doc) const
	{
		Object* contents = Null::get();
		if(!this->objects.empty())
		{
			auto strm = Stream::create(doc, {});
			for(auto obj : this->objects)
			{
				auto ser = obj->serialise(this);
				strm->append(reinterpret_cast<const uint8_t*>(ser.data()), ser.size());
			}

			contents = IndirectRef::create(strm);
		}

		// need to do the fonts only after the objects, because serialising objects
		// can use fonts.
		auto font_dict = Dictionary::create({});
		for(auto font : this->fonts)
		{
			auto fser = font->serialise(doc);
			font_dict->add(Name(font->getFontResourceName()), IndirectRef::create(fser));
		}

		auto resources = Dictionary::create({});
		if(!font_dict->values.empty())
			resources->add(names::Font, font_dict);


		return Dictionary::createIndirect(doc, names::Page,
			{ { names::Resources, resources }, { names::MediaBox, a4paper }, { names::Contents, contents } });
	}

	void Page::useFont(const Font* font) const
	{
		if(std::find(this->fonts.begin(), this->fonts.end(), font) != this->fonts.end())
			return;

		this->fonts.push_back(font);
	}

	void Page::addObject(PageObject* pobj)
	{
		this->objects.push_back(pobj);
	}

	PageObject::~PageObject()
	{
	}
}
