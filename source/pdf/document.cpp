// document.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for checked_cast

#include "pdf/misc.h"     // for error
#include "pdf/page.h"     // for Page
#include "pdf/font.h"     // for Font
#include "pdf/object.h"   // for Object, Dictionary, Name, IndirectRef, Int...
#include "pdf/writer.h"   // for Writer
#include "pdf/document.h" // for Document

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
		for(auto [_, obj] : m_objects)
			obj->writeFull(w);

		// write the xref table
		auto xref_position = w->position();

		auto num_objects = m_current_id + 1;

		// note: use \r\n line endings here so we can trim trailing whitespace
		// (ie. hand-edit the pdf) and not completely break it.
		w->writeln("xref");
		w->writeln("0 {}", num_objects);
		w->writeln("{010} {05} f\r", num_objects, 0xffff);

		for(size_t i = 0; i < num_objects; i++)
		{
			if(auto it = m_objects.find(i); it != m_objects.end())
				w->writeln("{010} {05} n\r", it->second->byteOffset(), it->second->gen());
		}

		w->writeln();

		auto trailer = Dictionary::create({ { names::Size, Integer::create(util::checked_cast<int64_t>(num_objects)) },
		    { names::Root, IndirectRef::create(root) } });

		w->writeln("trailer");
		w->write(trailer);

		w->writeln();
		w->writeln("startxref");
		w->writeln("{}", xref_position);
		w->writeln("%%EOF");
	}


	void Document::addPage(Page* page)
	{
		m_pages.push_back(page);
	}

	Dictionary* Document::createPageTree()
	{
		// TODO: make this more efficient -- make some kind of balanced tree.

		auto pagetree = Dictionary::createIndirect(this, names::Pages,
		    { { names::Count, Integer::create(util::checked_cast<int64_t>(m_pages.size())) } });

		auto array = Array::create({});
		for(auto page : m_pages)
		{
			auto obj = page->serialise(this);
			obj->addOrReplace(names::Parent, pagetree);
			array->append(IndirectRef::create(obj));
		}

		pagetree->addOrReplace(names::Kids, array);

		// serialise the fonts
		for(auto page : m_pages)
		{
			for(auto font : page->usedFonts())
			{
				if(not font->didSerialise())
					font->serialise(this);
			}
		}

		return pagetree;
	}







	Document::Document()
	{
		m_current_id = 0;
	}

	void Document::addObject(Object* obj)
	{
		if(not obj->isIndirect())
			pdf::error("cannot add non-indirect objects directly to Document");

		if(m_objects.find(obj->id()) != m_objects.end())
			pdf::error("object id '{}' already exists (generations not supported)", obj->id());

		m_objects.emplace(obj->id(), obj);
	}

	size_t Document::getNewObjectId()
	{
		return ++m_current_id;
	}

	size_t Document::getNextFontResourceNumber()
	{
		return ++m_current_font_number;
	}
}
