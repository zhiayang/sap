// page.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/font.h"        // for Font, File
#include "pdf/page.h"        // for Page
#include "pdf/units.h"       // for pdf_typographic_unit, Size2d, Vector2_Y...
#include "pdf/object.h"      // for Name, Dictionary, Object, IndirectRef
#include "pdf/xobject.h"     //
#include "pdf/page_object.h" // for PageObject

namespace pdf
{
	// TODO: support custom paper sizes
	static const auto a4paper = Array::create(Integer::create(0), Integer::create(0), Decimal::create(595.276),
	    Decimal::create(841.89));

	Page::Page() : m_dictionary(Dictionary::createIndirect(names::Page, {}))
	{
		m_page_size = pdf::Size2d(595.276, 841.89);
	}

	void Page::addFont(const PdfFont* font) const
	{
		m_fonts.insert(font);
	}

	void Page::addXObject(const XObject* xobject) const
	{
		m_xobjects.insert(xobject);
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

	void Page::serialiseResources() const
	{
		for(auto font : m_fonts)
			font->serialise();

		for(auto xobj : m_xobjects)
			xobj->serialise();
	}

	void Page::serialise() const
	{
		Object* contents = Null::get();
		if(not m_objects.empty())
		{
			auto strm = Stream::create({});
			for(auto obj : m_objects)
			{
				// ask the object to add whatever resources it needs
				obj->addResources(this);

				auto ser = obj->pdfRepresentation();
				strm->append(reinterpret_cast<const uint8_t*>(ser.data()), ser.size());
			}

			contents = IndirectRef::create(strm);
		}

		auto font_dict = Dictionary::create({});
		for(auto font : m_fonts)
			font_dict->add(Name(font->getFontResourceName()), IndirectRef::create(font->dictionary()));

		auto xobject_dict = Dictionary::create({});
		for(auto xobj : m_xobjects)
			xobject_dict->add(Name(xobj->getResourceName()), IndirectRef::create(xobj->stream()));

		auto resources = Dictionary::create({});
		resources->add(names::Font, font_dict);
		resources->add(names::XObject, xobject_dict);

		m_dictionary->addOrReplace(names::Resources, resources);
		m_dictionary->addOrReplace(names::MediaBox, a4paper);
		m_dictionary->addOrReplace(names::Contents, contents);
	}

	void Page::addObject(PageObject* pobj)
	{
		m_objects.push_back(pobj);
	}

	PageObject::~PageObject()
	{
	}
}
