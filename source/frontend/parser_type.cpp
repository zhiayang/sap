// parser_type.cpp
// Copyright (c) 2022, zhiayang
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
