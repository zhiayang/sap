// arrayType.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/type.h"

namespace sap::interp
{
	std::string ArrayType::str() const
	{
		if(m_is_variadic)
			return zpr::sprint("[{}...]", m_element_type);
		else
			return zpr::sprint("[{}]", m_element_type);
	}

	ArrayType::ArrayType(const Type* element_type, bool is_variadic)
		: Type(Type::KIND_ARRAY), m_element_type(element_type), m_is_variadic(is_variadic)
	{
	}

	bool ArrayType::sameAs(const Type* other) const
	{
		if(not other->isArray())
			return false;

		auto of = other->toArray();
		return m_element_type == of->m_element_type && m_is_variadic == of->m_is_variadic;
	}
}
