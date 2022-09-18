// function.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/type.h"

namespace sap::interp
{
	std::string FunctionType::str() const
	{
		std::string args {};
		for(size_t i = 0; i < m_params.size(); i++)
		{
			if(i != 0)
				args += ", ";
			args += m_params[i]->str();
		}
		return zpr::sprint("fn({}) -> {}", args, m_return_type);
	}

	FunctionType::FunctionType(std::vector<const Type*> params, const Type* return_type)
		: Type(Type::KIND_FUNCTION), m_params(std::move(params)), m_return_type(return_type)
	{
	}

	bool FunctionType::sameAs(const Type* other) const
	{
		if(not other->isFunction())
			return false;

		auto of = other->toFunction();
		return m_params == of->m_params && m_return_type == of->m_return_type;
	}
}
