// pointer.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/type.h"

namespace sap::interp
{
	std::string PointerType::str() const
	{
		if(m_is_mutable)
			return zpr::sprint("&mut {}", m_element_type);
		else
			return zpr::sprint("&{}", m_element_type);
	}

	bool PointerType::sameAs(const Type* other) const
	{
		if(not other->isPointer())
			return false;

		auto of = other->toPointer();
		return m_element_type == of->m_element_type && m_is_mutable == of->m_is_mutable;
	}
}
