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
		std::vector<Value> fields {};

		for(size_t i = 0; i < m_fields.size(); i++)
		{
			auto a = m_type->getFieldAtIndex(i);
			if(not m_fields[i].has_value())
			{
				if(not a->isOptional())
					sap::internal_error("unset field '{}'", m_type->getFields()[i].name);

				// it's optional -- make it null.
				fields.push_back(Value::optional(a->optionalElement(), std::nullopt));
			}
			else
			{
				auto b = m_fields[i]->type();
				if(a == b)
					fields.push_back(std::move(*m_fields[i]));
				else if(a->isOptional() && a->optionalElement() == b)
					fields.push_back(Value::optional(a->optionalElement(), std::move(*m_fields[i])));
				else
					sap::internal_error("mismatched field {} ({}, {})", m_type->getFields()[i].name, a, b);
			}
		}

		return Value::structure(m_type, std::move(fields));
	}

	StructMaker& StructMaker::set(zst::str_view name, Value value)
	{
		if(not m_type->hasFieldNamed(name))
			sap::internal_error("struct '{}' has no field named '{}'", (const Type*) m_type, name);

		m_fields[m_type->getFieldIndex(name)] = std::move(value);
		return *this;
	}
}
