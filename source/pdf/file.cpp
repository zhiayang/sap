// file.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for checked_cast

#include "pdf/file.h"   // for File
#include "pdf/font.h"   // for Font
#include "pdf/misc.h"   // for error
#include "pdf/page.h"   // for Page
#include "pdf/object.h" // for Object, Dictionary, Name, IndirectRef, Int...
#include "pdf/writer.h" // for Writer


namespace pdf
{
	void File::write(Writer* w)
	{
		w->writeln("%PDF-1.7");

		// add 4 non-ascii bytes to signify a binary file
		// (since we'll probably be embedding fonts and other stuff)
		w->writeln("%\xf0\xf1\xf2\xf3");
		w->writeln();

		auto pagetree = this->createPageTree();
		auto root = Dictionary::createIndirect(names::Catalog, { { names::Pages, IndirectRef::create(pagetree) } });

		// first, traverse all objects that are reachable from the root
		root->collectIndirectObjectsAndAssignIds(this);

		// then write the indirect objects
		root->writeIndirectObjects(w);

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


	void File::addPage(Page* page)
	{
		m_pages.push_back(page);
	}

	Dictionary* File::createPageTree()
	{
		// TODO: make this more efficient -- make some kind of balanced tree.

		auto pagetree = Dictionary::createIndirect(names::Pages,
		    { { names::Count, Integer::create(util::checked_cast<int64_t>(m_pages.size())) } });

		auto array = Array::create({});
		for(auto page : m_pages)
		{
			auto obj = page->serialise();
			obj->addOrReplace(names::Parent, pagetree);
			array->append(IndirectRef::create(obj));
		}

		pagetree->addOrReplace(names::Kids, array);

		// figure out which fonts were used.
		std::unordered_set<const PdfFont*> used_fonts {};
		for(auto page : m_pages)
			used_fonts.insert(page->usedFonts().begin(), page->usedFonts().end());

		for(auto font : used_fonts)
			font->serialise();

		return pagetree;
	}







	File::File()
	{
		m_current_id = 0;
	}

	void File::addObject(Object* obj)
	{
		if(not obj->isIndirect())
			pdf::error("cannot add non-indirect objects directly to Document");

		if(m_objects.find(obj->id()) != m_objects.end())
			pdf::error("object id '{}' already exists (generations not supported)", obj->id());

		m_objects.emplace(obj->id(), obj);
	}

	size_t File::getNewObjectId()
	{
		return ++m_current_id;
	}

	size_t File::getNextFontResourceNumber()
	{
		return ++m_current_font_number;
	}
}
