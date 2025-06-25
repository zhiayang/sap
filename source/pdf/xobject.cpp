// xobject.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "pdf/page.h"
#include "pdf/object.h"
#include "pdf/xobject.h"

namespace pdf
{
	XObject::XObject(const Name& subtype) : Resource(KIND_XOBJECT), m_stream(Stream::create())
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

	Object* XObject::resourceObject() const
	{
		return m_stream;
	}

	void XObject::addResources(const Page* page) const
	{
		page->addResource(this);
	}
}
