// page.h
// Copyright (c) 2021, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/units.h"
#include "pdf/object.h"
#include "pdf/page_object.h"

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
		mutable util::hashset<const Resource*> m_resources;

		Size2d m_page_size {};
	};
}
