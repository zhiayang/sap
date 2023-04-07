// page.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/units.h"       // for Size2d, Vector2_YDown, Vector2_YUp
#include "pdf/object.h"      // for Dictionary, File
#include "pdf/page_object.h" // for PageObject

namespace pdf
{
	struct File;
	struct Resource;
	struct Annotation;

	struct Page
	{
		Page();

		void serialise(File* file) const;
		void serialiseResources() const;

		void addObject(PageObject* obj);

		Size2d size() const;

		Vector2_YUp convertVector2(Vector2_YDown v2) const;
		Vector2_YDown convertVector2(Vector2_YUp v2) const;

		void addResource(const Resource* resource) const;
		void addAnnotation(const Annotation* annotation);

		Dictionary* dictionary() const { return m_dictionary; }

	private:
		Dictionary* m_dictionary;
		std::vector<PageObject*> m_objects;
		std::vector<const Annotation*> m_annotations;
		mutable std::unordered_set<const Resource*> m_resources;

		Size2d m_page_size {};
	};
}
