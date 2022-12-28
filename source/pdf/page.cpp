// page.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <algorithm> // for find

#include "pdf/font.h"        // for Font, Document
#include "pdf/page.h"        // for Page
#include "pdf/units.h"       // for Size2d, Vector2_YDown, Vector2_YUp, pdf_...
#include "pdf/object.h"      // for Name, Dictionary, Object, IndirectRef
#include "pdf/page_object.h" // for PageObject

namespace pdf
{
	// TODO: support custom paper sizes
	static const auto a4paper = Array::create(Integer::create(0), Integer::create(0), Decimal::create(595.276),
	    Decimal::create(841.89));

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
		return Vector2_YUp(v2.x(), m_page_size.y() - v2.y());
	}

	Vector2_YDown Page::convertVector2(Vector2_YUp v2) const
	{
		return Vector2_YDown(v2.x(), m_page_size.y() - v2.y());
	}


	Dictionary* Page::serialise(File* doc) const
	{
		Object* contents = Null::get();
		if(not m_objects.empty())
		{
			auto strm = Stream::create(doc, {});
			for(auto obj : m_objects)
			{
				auto ser = obj->serialise(this);
				strm->append(reinterpret_cast<const uint8_t*>(ser.data()), ser.size());
			}

			contents = IndirectRef::create(strm);
		}

		// need to do the fonts only after the objects, because serialising objects
		// can use fonts.
		auto font_dict = Dictionary::create({});
		for(auto font : m_used_fonts)
			font_dict->add(Name(font->getFontResourceName()), IndirectRef::create(font->dictionary()));

		auto resources = Dictionary::create({});
		if(not font_dict->empty())
			resources->add(names::Font, font_dict);

		return Dictionary::createIndirect(doc, names::Page,
		    { { names::Resources, resources }, { names::MediaBox, a4paper }, { names::Contents, contents } });
	}

	void Page::useFont(const Font* font) const
	{
		if(std::find(m_used_fonts.begin(), m_used_fonts.end(), font) != m_used_fonts.end())
			return;

		m_used_fonts.push_back(font);
	}

	void Page::addObject(PageObject* pobj)
	{
		m_objects.push_back(pobj);
	}

	PageObject::~PageObject()
	{
	}
}
