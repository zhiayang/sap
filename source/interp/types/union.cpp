// union.cpp
// Copyright (c) 2023, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/type.h"

namespace sap::interp
{
	UnionType::UnionType(QualifiedId name, std::vector<Case> cases)
	    : Type(KIND_UNION), m_name(std::move(name)), m_cases(std::move(cases))
	{
		size_t i = 0;
		for(auto& [n, t] : m_cases)
			m_case_map[n] = { i++, t };
	}

	void UnionType::setCases(std::vector<Case> cases)
	{
		m_cases = std::move(cases);

		size_t i = 0;
		for(auto& [n, t] : m_cases)
			m_case_map[n] = { i++, t };
	}

	std::string UnionType::str() const
	{
		return zpr::sprint("union({})", m_name.name);
	}

	bool UnionType::sameAs(const Type* other) const
	{
		return other->isUnion() && other->toUnion()->name() == m_name;
	}

	bool UnionType::hasCaseNamed(zst::str_view name) const
	{
		return m_case_map.contains(name);
	}

	size_t UnionType::getCaseIndex(zst::str_view name) const
	{
		auto it = m_case_map.find(name);
		assert(it != m_case_map.end());

		return it->second.first;
	}

	const StructType* UnionType::getCaseNamed(zst::str_view name) const
	{
		auto it = m_case_map.find(name);
		assert(it != m_case_map.end());

		return it->second.second;
	}

	const StructType* UnionType::getCaseAtIndex(size_t idx) const
	{
		assert(idx < m_cases.size());
		return m_cases[idx].type;
	}


	auto UnionType::getCases() const -> const std::vector<Case>&
	{
		return m_cases;
	}

	std::vector<const StructType*> UnionType::getCaseTypes() const
	{
		std::vector<const StructType*> types {};
		for(auto& [n, t] : m_cases)
			types.push_back(t);

		return types;
	}
}
