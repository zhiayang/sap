// page.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "pdf/page.h"
#include "pdf/font.h"
#include "pdf/misc.h"
#include "pdf/object.h"
#include "pdf/pageobject.h"

namespace pdf
{
	static const auto a4paper = Array::create(Integer::create(0), Integer::create(0), Decimal::create(595.276), Decimal::create(841.89));

	Dictionary* Page::serialise(Document* doc) const
	{
		Object* contents = Null::get();
		if(!this->objects.empty())
		{
			auto strm = Stream::create(doc, { });
			for(auto obj : this->objects)
			{
				auto ser = obj->serialise(this);
				strm->append(reinterpret_cast<const uint8_t*>(ser.data()), ser.size());
			}

			contents = IndirectRef::create(strm);
		}

		// need to do the fonts only after the objects, because serialising objects
		// can use fonts.
		auto font_dict = Dictionary::create({ });
		for(size_t i = 0; i < this->fonts.size(); i++)
		{
			auto f = this->fonts[i]->serialise(doc);
			font_dict->add(Name(zpr::sprint("F{}", i)), IndirectRef::create(f));
		}

		auto resources = Dictionary::create({ });
		if(!font_dict->values.empty())
			resources->add(names::Font, font_dict);


		return Dictionary::createIndirect(doc, names::Page, {
			{ names::Resources, resources },
			{ names::MediaBox, a4paper },
			{ names::Contents, contents }
		});
	}

	void Page::useFont(Font* font)
	{
		if(std::find(this->fonts.begin(), this->fonts.end(), font) != this->fonts.end())
			pdf::error("page already contains font");

		this->fonts.push_back(font);
	}

	std::string Page::getNameForFont(Font* font) const
	{
		auto it = std::find(this->fonts.begin(), this->fonts.end(), font);
		if(it != this->fonts.end())
			return zpr::sprint("F{}", it - this->fonts.begin());

		size_t n = this->fonts.size();
		this->fonts.push_back(font);
		return zpr::sprint("F{}", n);
	}

	void Page::addObject(PageObject* pobj)
	{
		this->objects.push_back(pobj);
	}

	PageObject::~PageObject() { }
}
