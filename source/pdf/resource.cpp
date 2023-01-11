// resource.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/object.h"
#include "pdf/resource.h"

namespace pdf
{
	Resource::~Resource()
	{
	}

	Resource::Resource(Kind kind) : m_kind(kind)
	{
		m_name = zpr::sprint("{}{}", this->resourceKindString(), pdf::getNewResourceId());
	}

	zst::str_view Resource::resourceName() const
	{
		return m_name;
	}

	zst::str_view Resource::resourceKindString() const
	{
		switch(m_kind)
		{
			case KIND_FONT: return "Font";
			case KIND_XOBJECT: return "XObject";
			case KIND_EXTGSTATE: return "ExtGState";
		}
	}
}
