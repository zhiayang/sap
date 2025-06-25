// resource.cpp
// Copyright (c) 2022, yuki
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
		auto ks = this->resourceKindString();
		const char* prefix = "U";
		switch(m_kind)
		{
			case KIND_FONT: prefix = "F"; break;
			case KIND_XOBJECT: prefix = "x"; break;
			case KIND_EXTGSTATE: prefix = "g"; break;
		}

		m_name = zpr::sprint("{}{}", prefix, pdf::getNewResourceId(/* key: */ ks));
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
		util::unreachable();
	}
}
