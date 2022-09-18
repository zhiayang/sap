// document.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

#include "pdf/object.h"

namespace pdf
{
	struct Page;

	struct Writer;
	struct Object;

	struct Document
	{
		Document();

		Document(const Document&) = delete;
		Document& operator=(const Document&) = delete;

		Document(Document&&) = default;
		Document& operator=(Document&&) = default;

		void write(Writer* stream);
		void addObject(Object* obj);

		size_t getNewObjectId();

		void addPage(Page* page);

		size_t getNextFontResourceNumber();

	private:
		size_t current_id = 0;
		std::map<size_t, Object*> objects;

		std::vector<Page*> pages;

		size_t current_font_number = 0;
		Dictionary* createPageTree();
	};


	template <typename T, typename... Args, typename = std::enable_if_t<std::is_base_of_v<Object, T>>>
	inline T* createObject(Args&&... args)
	{
		return util::make<T>(static_cast<Args&&>(args)...);
	}

	template <typename T, typename... Args, typename = std::enable_if_t<std::is_base_of_v<Object, T>>>
	inline T* createIndirectObject(Document* doc, Args&&... args)
	{
		auto obj = util::make<T>(static_cast<Args&&>(args)...);
		obj->is_indirect = true;
		obj->id = doc->getNewObjectId();
		obj->gen = 0;

		doc->addObject(obj);
		return obj;
	}
}
