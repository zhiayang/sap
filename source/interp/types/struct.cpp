// struct.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/type.h"

namespace sap::interp
{
	StructType::StructType(QualifiedId name, std::vector<Field> fields)
	    : Type(KIND_STRUCT), m_name(std::move(name)), m_fields(std::move(fields))
	{
		size_t i = 0;
		for(auto& [n, t] : m_fields)
			m_field_map[n] = { i++, t };
	}

	void StructType::setFields(std::vector<Field> fields)
	{
		m_fields = std::move(fields);

		size_t i = 0;
		for(auto& [n, t] : m_fields)
			m_field_map[n] = { i++, t };
	}

	std::string StructType::str() const
	{
		return zpr::sprint("struct({})", m_name.name);
	}

	bool StructType::sameAs(const Type* other) const
	{
		return other->isStruct() && other->toStruct()->name() == m_name;
	}

	bool StructType::hasFieldNamed(zst::str_view name) const
	{
		return m_field_map.contains(name);
	}

	size_t StructType::getFieldIndex(zst::str_view name) const
	{
		auto it = m_field_map.find(name);
		assert(it != m_field_map.end());

		return it->second.first;
	}

	const Type* StructType::getFieldNamed(zst::str_view name) const
	{
		auto it = m_field_map.find(name);
		assert(it != m_field_map.end());

		return it->second.second;
	}

	const Type* StructType::getFieldAtIndex(size_t idx) const
	{
		assert(idx < m_fields.size());
		return m_fields[idx].type;
	}


	auto StructType::getFields() const -> const std::vector<Field>&
	{
		return m_fields;
	}

	std::vector<const Type*> StructType::getFieldTypes() const
	{
		std::vector<const Type*> types {};
		for(auto& [n, t] : m_fields)
			types.push_back(t);

		return types;
	}
}
