// graphics.cpp
// Copyright (c) 2023, yuki
// SPDX-License-Identifier: Apache-2.0

#include "pdf/object.h"
#include "pdf/resource.h"

namespace pdf
{
	ExtGraphicsState::ExtGraphicsState() : Resource(KIND_EXTGSTATE), m_dict(Dictionary::create({}))
	{
		m_dict->add(names::CA, Integer::create(1));
		m_dict->add(names::ca, Integer::create(1));
	}

	void ExtGraphicsState::serialise() const
	{
		// ?????
	}

	Object* ExtGraphicsState::resourceObject() const
	{
		return m_dict;
	}
}
