// file.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector> // for vector

#include "pdf/object.h" // for Object, Dictionary, Writer

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

		size_t getNextFontResourceNumber();

	private:
		size_t m_current_id = 0;
		std::map<size_t, Object*> m_objects;

		std::vector<Page*> m_pages;

		size_t m_current_font_number = 0;
		Dictionary* createPageTree();
	};

}
