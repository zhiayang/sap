// optional.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/type.h"

namespace sap::interp
{
	std::string OptionalType::str() const
	{
		return zpr::sprint("?{}", m_element_type);
	}

	bool OptionalType::sameAs(const Type* other) const
	{
		if(not other->isOptional())
			return false;

		auto of = other->toOptional();
		return m_element_type == of->m_element_type;
	}
}
