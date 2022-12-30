// parser_type.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

#include "interp/basedefs.h"

namespace sap::frontend
{
	inline constexpr const char* TYPE_ANY = "any";
	inline constexpr const char* TYPE_INT = "int";
	inline constexpr const char* TYPE_BOOL = "bool";
	inline constexpr const char* TYPE_CHAR = "char";
	inline constexpr const char* TYPE_VOID = "void";
	inline constexpr const char* TYPE_FLOAT = "float";
	inline constexpr const char* TYPE_STRING = "string";
	inline constexpr const char* TYPE_TREE_BLOCK = "Block";
	inline constexpr const char* TYPE_TREE_INLINE = "Inline";

	struct PType
	{
		bool isFunction() const { return m_kind == PT_FUNCTION; }
		bool isPointer() const { return m_kind == PT_POINTER; }
		bool isArray() const { return m_kind == PT_ARRAY; }

		bool isNamed() const { return not m_name.name.empty(); }
		const interp::QualifiedId& name() const { return m_name; }

		bool isVariadicArray() const { return m_kind == PT_ARRAY && m_array_variadic; }

		const std::vector<PType>& getTypeList() const { return m_type_list; }

		const PType& getArrayElement() const { return m_type_list[0]; }

		static PType named(const char* name)
		{
			return PType(
			    interp::QualifiedId {
			        .top_level = false,
			        .parents = {},
			        .name = name,
			    },
			    0, {});
		}

		static PType named(interp::QualifiedId qid) { return PType(std::move(qid), 0, {}); }

		static PType function(std::vector<PType> params, PType ret)
		{
			params.push_back(std::move(ret));
			return PType({}, PT_FUNCTION, std::move(params));
		}

		static PType array(PType elm, bool variadic)
		{
			auto ret = PType({}, PT_ARRAY, { elm });
			ret.m_array_variadic = variadic;
			return ret;
		}

	private:
		PType(interp::QualifiedId name, int kind, std::vector<PType> type_list)
		    : m_kind(kind)
		    , m_name(std::move(name))
		    , m_type_list(std::move(type_list))
		    , m_array_variadic(false)
		{
		}

		static constexpr int PT_FUNCTION = 0;
		static constexpr int PT_POINTER = 1;
		static constexpr int PT_ARRAY = 2;

		int m_kind;
		interp::QualifiedId m_name;
		std::vector<PType> m_type_list;
		bool m_array_variadic;
	};
}
