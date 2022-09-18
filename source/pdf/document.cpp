// document.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/page.h"
#include "pdf/misc.h"
#include "pdf/writer.h"
#include "pdf/object.h"
#include "pdf/document.h"

namespace pdf
{
	void Document::write(Writer* w)
	{
		w->writeln("%PDF-1.7");

		// add 4 non-ascii bytes to signify a binary file
		// (since we'll probably be embedding fonts and other stuff)
		w->writeln("%\xf0\xf1\xf2\xf3");
		w->writeln();

		auto pagetree = this->createPageTree();
		auto root = Dictionary::createIndirect(this, names::Catalog, { { names::Pages, IndirectRef::create(pagetree) } });

		// write all the objects.
		for(auto [_, obj] : this->objects)
			obj->writeFull(w);

		// write the xref table
		auto xref_position = w->position();

		auto num_objects = this->current_id + 1;

		// note: use \r\n line endings here so we can trim trailing whitespace
		// (ie. hand-edit the pdf) and not completely break it.
		w->writeln("xref");
		w->writeln("0 {}", num_objects);
		w->writeln("{010} {05} f\r", num_objects, 0xffff);

		for(size_t i = 0; i < num_objects; i++)
		{
			if(auto it = this->objects.find(i); it != this->objects.end())
				w->writeln("{010} {05} n\r", it->second->byte_offset, it->second->gen);
		}

		w->writeln();

		auto trailer =
			Dictionary::create({ { names::Size, Integer::create(num_objects) }, { names::Root, IndirectRef::create(root) } });

		w->writeln("trailer");
		w->write(trailer);

		w->writeln();
		w->writeln("startxref");
		w->writeln("{}", xref_position);
		w->writeln("%%EOF");
	}


	void Document::addPage(Page* page)
	{
		this->pages.push_back(page);
	}

	Dictionary* Document::createPageTree()
	{
		// TODO: make this more efficient -- make some kind of balanced tree.

		auto pagetree = Dictionary::createIndirect(this, names::Pages, { { names::Count, Integer::create(this->pages.size()) } });

		auto array = Array::create({});
		for(auto page : this->pages)
		{
			auto obj = page->serialise(this);
			obj->addOrReplace(names::Parent, pagetree);
			array->values.push_back(IndirectRef::create(obj));
		}

		pagetree->addOrReplace(names::Kids, array);

		return pagetree;
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

	size_t Document::getNewObjectId()
	{
		return ++this->current_id;
	}

	size_t Document::getNextFontResourceNumber()
	{
		return ++this->current_font_number;
	}
}
