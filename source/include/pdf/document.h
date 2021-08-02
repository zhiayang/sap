// document.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <vector>

#include "pdf/object.h"

namespace pdf
{
	struct Writer;
	struct Object;

	struct Document
	{
		Document();

		void write(Writer* stream);

		void setRoot(Object* obj);
		void addObject(Object* obj);

		size_t getNewObjectId();

	private:
		Object* root_object;
		std::map<size_t, Object*> objects;

		size_t current_id = 0;
	};


	template <typename T, typename... Args, typename = std::enable_if_t<!std::is_same_v<Stream, T> && std::is_base_of_v<Object, T>>>
	inline T* createObject(Args&&... args)
	{
		static_assert(!std::is_same_v<T, Stream>);
		return util::make<T>(static_cast<Args&&>(args)...);
	}

	template <typename T, typename... Args, typename = std::enable_if_t<std::is_same_v<Stream, T> && std::is_base_of_v<Object, T>>>
	inline T* createObject(Document* doc, Args&&... args)
	{
		static_assert(std::is_same_v<T, Stream>);

		auto obj = util::make<T>(static_cast<Args&&>(args)...);
		obj->is_indirect = true;
		obj->id = doc->getNewObjectId();
		obj->gen = 0;

		doc->addObject(obj);
		return obj;
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
