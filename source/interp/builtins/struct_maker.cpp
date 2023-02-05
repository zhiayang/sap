// struct_maker.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"
#include "layout/base.h"

#include "interp/ast.h"
#include "interp/value.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	StructMaker::StructMaker(const StructType* type) : m_type(type)
	{
		m_fields.resize(type->getFields().size());
	}

	Value StructMaker::make()
	{
		// do some things...
		for(size_t i = 0; i < m_fields.size(); i++)
		{
			auto a = m_type->getFieldAtIndex(i);
			auto b = m_fields[i].type();

			if(a == b)
				continue;

			else if(a->isOptional() && a->optionalElement() == b)
				m_fields[i] = Value::optional(a->optionalElement(), std::move(m_fields[i]));
		}

		return Value::structure(m_type, std::move(m_fields));
	}

	StructMaker& StructMaker::set(zst::str_view name, Value value)
	{
		if(not m_type->hasFieldNamed(name))
			sap::internal_error("struct '{}' has no field named '{}'", (const Type*) m_type, name);

		m_fields[m_type->getFieldIndex(name)] = std::move(value);
		return *this;
	}
}
