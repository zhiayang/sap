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

#if !defined(GIT_REVISION)
#define GIT_REVISION "unknown"
#endif

namespace pdf
{
	void File::write(Writer* w)
	{
		w->writeln("%PDF-1.7");

		// add 4 non-ascii bytes to signify a binary file
		// (since we'll probably be embedding fonts and other stuff)
		w->writeln("%\xf0\xf1\xf2\xf3");
		w->writeln();

		auto pagetree = this->create_page_tree();
		auto root = Dictionary::createIndirect(names::Catalog, { { names::Pages, IndirectRef::create(pagetree) } });

		if(not m_outline_items.empty())
		{
			auto outlines_root = Dictionary::createIndirect(names::Outlines, {});
			auto [first_child, last_child] = OutlineItem::linkChildren(m_outline_items, outlines_root, this);

			outlines_root->add(names::Count, Integer::create(checked_cast<int64_t>(m_outline_items.size())));
			outlines_root->add(names::First, first_child);
			outlines_root->add(names::Last, last_child);

			root->add(names::Outlines, outlines_root);

			// show outlines by default if we have it
			root->add(names::PageMode, names::UseOutlines.ptr());
		}

		auto info_dict = Dictionary::createIndirect({
			{ names::Creator, String::create("sap-" GIT_REVISION) },
			{ names::Producer, String::create("sap-" GIT_REVISION) },
		});

		auto x = LinkAnnotation({ 50, 50 }, { 50, 50 },
			Destination {
				.page = 2,
				.zoom = 0,
				.position = { 100, 100 },
			});

		m_pages[0]->dictionary()->add(names::Annots, Array::create(x.toDictionary(this)));




		// first, traverse all objects that are reachable from the root
		root->collectIndirectObjectsAndAssignIds(this);
		info_dict->collectIndirectObjectsAndAssignIds(this);

		// then write the indirect objects
		root->writeIndirectObjects(w);
		info_dict->writeIndirectObjects(w);

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

		auto trailer = Dictionary::create({
			{ names::Size, Integer::create(util::checked_cast<int64_t>(num_objects)) },
			{ names::Info, IndirectRef::create(info_dict) },
			{ names::Root, IndirectRef::create(root) },
		});

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

	Dictionary* File::create_page_tree()
	{
		// TODO: make this more efficient -- make some kind of balanced tree.

		auto pagetree = Dictionary::createIndirect(names::Pages,
			{ { names::Count, Integer::create(util::checked_cast<int64_t>(m_pages.size())) } });

		auto array = Array::create({});
		for(auto page : m_pages)
		{
			page->serialise();
			page->serialiseResources();

			page->dictionary()->addOrReplace(names::Parent, pagetree);

			array->append(IndirectRef::create(page->dictionary()));
		}

		pagetree->addOrReplace(names::Kids, array);

		return pagetree;
	}







	File::File()
	{
		m_current_id = 0;
	}

	void File::addOutlineItem(OutlineItem outline_item)
	{
		m_outline_items.push_back(std::move(outline_item));
	}

	Page* File::getPage(size_t num) const
	{
		if(num >= m_pages.size())
			pdf::error("page number {} is out of range (have only {})", num, m_pages.size());

		return m_pages[num];
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
