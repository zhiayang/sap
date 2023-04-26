// enum.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/type.h"

namespace sap::interp
{
	std::string EnumType::str() const
	{
		return zpr::sprint("enum({}: {})", m_name.name, m_element_type);
	}

	bool EnumType::sameAs(const Type* other) const
	{
		if(not other->isEnum())
			return false;

		auto of = other->toEnum();
		return m_element_type == of->m_element_type && m_name == of->m_name;
	}
}
