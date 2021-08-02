// document.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "pdf/misc.h"
#include "pdf/writer.h"
#include "pdf/object.h"
#include "pdf/document.h"

namespace pdf
{
	void Document::write(Writer* w)
	{
		const auto a4paper = Array::create(Integer::create(0), Integer::create(0), Decimal::create(595.276), Decimal::create(841.89));

		w->writeln("%PDF-1.7");

		// add 4 non-ascii bytes to signify a binary file
		// (since we'll probably be embedding fonts and other stuff)
		w->writeln("%\xf0\xf1\xf2\xf3");
		w->writeln("");

		auto page = Dictionary::createIndirect(this, names::Page, {
			{ names::Resources, Dictionary::create({ }) },
			{ names::MediaBox, a4paper }
		});

		auto pagetree = Dictionary::createIndirect(this, names::Pages, {
			{ names::Count, Integer::create(1) },
			{ names::Kids, Array::create(IndirectRef::create(page)) }
		});

		page->addOrReplace(names::Parent, pagetree);

		auto root = Dictionary::createIndirect(this, names::Catalog, {
			{ names::Pages, IndirectRef::create(pagetree) }
		});

		this->setRoot(root);

		// write all the objects.
		for(auto [ _, obj ] : this->objects)
			obj->writeFull(w);

		// write the xref table
		auto xref_position = w->position();

		w->writeln("xref");
		w->writeln("0 {}", this->current_id + 1);
		w->writeln("{010} {05} f ", this->current_id + 1, 0xffff);

		for(size_t i = 0; i <= this->current_id; i++)
		{
			if(auto it = this->objects.find(i); it != this->objects.end())
				w->writeln("{010} {05} n ", it->second->byte_offset, it->second->gen);
		}

		w->writeln();

		auto trailer = Dictionary::create({
			{ names::Size, Integer::create(this->current_id + 1) },
			{ names::Root, IndirectRef::create(root) }
		});

		w->writeln("trailer");
		w->write(trailer);

		w->writeln("startxref");
		w->writeln("{}", xref_position);

		w->writeln("%%EOF");
	}











	Document::Document()
	{
		this->current_id = 0;
	}

	void Document::addObject(Object* obj)
	{
		if(!obj->is_indirect)
			pdf::error("cannot add non-indirect objects directly to Document");

		if(this->objects.find(obj->id) != this->objects.end())
			pdf::error("object id '{}' already exists (generations not supported)", obj->id);

		this->objects.emplace(obj->id, obj);
	}

	void Document::setRoot(Object* obj)
	{
		// TODO: verify that the root object is valid
		this->root_object = obj;
	}

	size_t Document::getNewObjectId()
	{
		return ++this->current_id;
	}
}
