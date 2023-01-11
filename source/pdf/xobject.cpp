// xobject.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/page.h"
#include "pdf/object.h"
#include "pdf/xobject.h"

namespace pdf
{
	XObject::XObject(const Name& subtype)
	    : m_stream(Stream::create())
	    , m_resource_name(zpr::sprint("X{}", pdf::getNewResourceId()))
	{
		m_stream->dictionary()->add(names::Type, names::XObject.ptr());
		m_stream->setCompressed(true);
	}


	void XObject::serialise() const
	{
		if(m_did_serialise)
			return;

		m_did_serialise = true;
	}

	void XObject::addResources(const Page* page) const
	{
		page->addXObject(this);
	}

	const std::string& XObject::getResourceName() const
	{
		return m_resource_name;
	}
}
