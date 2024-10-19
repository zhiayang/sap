// page.cpp
// Copyright (c) 2021, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/font.h"        // for Font, File
#include "pdf/page.h"        // for Page
#include "pdf/units.h"       // for pdf_typographic_unit, Size2d, Vector2_Y...
#include "pdf/object.h"      // for Name, Dictionary, Object, IndirectRef
#include "pdf/xobject.h"     //
#include "pdf/annotation.h"  //
#include "pdf/page_object.h" // for PageObject

namespace pdf
{
	// TODO: support custom paper sizes
	static const auto a4paper = Array::
		create(Integer::create(0), Integer::create(0), Decimal::create(595.276), Decimal::create(841.89));

	Page::Page() : m_dictionary(Dictionary::createIndirect(names::Page, {}))
	{
		m_page_size = pdf::Size2d(595.276, 841.89);
	}

	void Page::addResource(const Resource* font) const
	{
		m_resources.insert(font);
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
		for(auto res : m_resources)
			res->serialise();
	}

	void Page::serialise(File* file) const
	{
		Object* contents = Null::get();
		if(not m_objects.empty())
		{
			auto strm = Stream::create({});
			for(auto obj : m_objects)
			{
				// ask the object to add whatever resources it needs
				obj->addResources(this);
				obj->writePdfCommands(strm);
			}

			contents = IndirectRef::create(strm);
		}

		util::hashmap<std::string, Dictionary*> resource_dicts;
		for(auto res : m_resources)
		{
			auto res_name = Name(res->resourceName());
			auto res_kind = res->resourceKindString();
			if(auto it = resource_dicts.find(res_kind); it == resource_dicts.end())
				resource_dicts.emplace(res_kind.str(), Dictionary::create({ { res_name, res->resourceObject() } }));
			else
				resource_dicts[res_kind.str()]->add(res_name, res->resourceObject());
		}

		auto resources = Dictionary::create({});
		for(auto& [k, d] : resource_dicts)
			resources->add(Name(k), d);

		m_dictionary->addOrReplace(names::Resources, resources);
		m_dictionary->addOrReplace(names::MediaBox, a4paper);
		m_dictionary->addOrReplace(names::Contents, contents);

		m_dictionary->addOrReplace(names::Annots, Array::create(util::map(m_annotations, [&](auto annot) -> Object* {
			return annot->toDictionary(file);
		})));
	}

	void Page::addAnnotation(const Annotation* annotation)
	{
		m_annotations.push_back(annotation);
	}

	void Page::addObject(PageObject* pobj)
	{
		m_objects.push_back(pobj);
	}

	PageObject::~PageObject()
	{
	}
}
