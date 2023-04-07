// file.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

#include "pdf/object.h"
#include "pdf/annotation.h"

namespace pdf
{
	struct Page;
	struct Writer;
	struct Object;

	struct File
	{
		File();

		File(const File&) = delete;
		File& operator=(const File&) = delete;

		File(File&&) = default;
		File& operator=(File&&) = default;

		void write(Writer* stream);
		void addObject(Object* obj);

		size_t getNewObjectId();

		void addPage(Page* page);
		Page* getPage(size_t page_num) const;

		void addOutlineItem(OutlineItem outline_item);

		size_t getNextFontResourceNumber();

	private:
		Dictionary* create_page_tree();

	private:
		size_t m_current_id = 0;
		util::hashmap<size_t, Object*> m_objects;

		std::vector<Page*> m_pages;
		std::vector<OutlineItem> m_outline_items;

		size_t m_current_font_number = 0;
	};

}
