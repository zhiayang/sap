// tree.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/tree.h"

namespace sap::tree
{
	void Document::addObject(std::shared_ptr<DocumentObject> obj)
	{
		m_objects.push_back(std::move(obj));
	}

	void Paragraph::addObject(std::shared_ptr<InlineObject> obj)
	{
		m_contents.push_back(std::move(obj));
	}













	DocumentObject::~DocumentObject()
	{
	}
	InlineObject::~InlineObject()
	{
	}
	BlockObject::~BlockObject()
	{
	}
}
