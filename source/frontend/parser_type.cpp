// parser_type.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/parser_type.h"

namespace sap::frontend
{
	bool PType::isNamed() const
	{
		return not m_name.name.empty();
	}
	const interp::QualifiedId& PType::name() const
	{
		return m_name;
	}

	const PType& PType::getArrayElement() const
	{
		return m_type_list[0];
	}
	const PType& PType::getPointerElement() const
	{
		return m_type_list[0];
	}
	const PType& PType::getOptionalElement() const
	{
		return m_type_list[0];
	}
	const std::vector<PType>& PType::getTypeList() const
	{
		return m_type_list;
	}

	PType PType::named(interp::QualifiedId qid)
	{
		return PType(std::move(qid), 0, {});
	}

	PType PType::named(const char* name)
	{
		return PType(
		    interp::QualifiedId {
		        .top_level = false,
		        .parents = {},
		        .name = name,
		    },
		    0, {});
	}

	PType PType::function(std::vector<PType> params, PType ret)
	{
		params.push_back(std::move(ret));
		return PType({}, PT_FUNCTION, std::move(params));
	}

	PType PType::array(PType elm, bool variadic)
	{
		auto ret = PType({}, PT_ARRAY, { elm });
		ret.m_array_variadic = variadic;
		return ret;
	}

	PType PType::variadicArray(PType elm)
	{
		auto ret = PType({}, PT_ARRAY, { elm });
		ret.m_array_variadic = true;
		return ret;
	}

	PType PType::pointer(PType elm, bool is_mut)
	{
		auto ret = PType({}, PT_POINTER, { elm });
		ret.m_pointer_mutable = is_mut;
		return ret;
	}

	PType PType::optional(PType elm)
	{
		return PType({}, PT_OPTIONAL, { elm });
	}

	std::string PType::str() const
	{
		if(this->isNamed())
			return this->name().str();
		else if(this->isPointer())
			return zpr::sprint("&{}{}", m_pointer_mutable ? "mut " : "", m_type_list[0].str());
		else if(this->isArray())
			return zpr::sprint("[{}{}]", m_type_list[0].str(), m_array_variadic ? "..." : "");
		else if(this->isOptional())
			return zpr::sprint("?{}", m_type_list[0].str());

		assert(this->isFunction());
		std::string ret = "(";
		for(size_t i = 0; i < m_type_list.size() - 1; i++)
		{
			if(i != 0)
				ret += ", ";
			ret += m_type_list[i].str();
		}

		ret += " -> " + m_type_list.back().str();
		return ret;
	}

	PType::~PType()
	{
	}

	PType::PType(interp::QualifiedId name, int kind, std::vector<PType> type_list)
	    : m_kind(kind)
	    , m_name(std::move(name))
	    , m_type_list(std::move(type_list))
	    , m_array_variadic(false)
	    , m_pointer_mutable(false)
	{
	}

}
