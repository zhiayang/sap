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
	inline constexpr const char* TYPE_LENGTH = "Length";
	inline constexpr const char* TYPE_TREE_BLOCK = "Block";
	inline constexpr const char* TYPE_TREE_INLINE = "Inline";
	inline constexpr const char* TYPE_LAYOUT_OBJECT = "LayoutObject";
	inline constexpr const char* TYPE_TREE_BLOCK_REF = "BlockRef";
	inline constexpr const char* TYPE_TREE_INLINE_REF = "InlineRef";
	inline constexpr const char* TYPE_LAYOUT_OBJECT_REF = "LayoutObjectRef";

	struct PType
	{
		bool isFunction() const { return m_kind == PT_FUNCTION; }
		bool isOptional() const { return m_kind == PT_OPTIONAL; }
		bool isPointer() const { return m_kind == PT_POINTER; }
		bool isArray() const { return m_kind == PT_ARRAY; }

		bool isVariadicArray() const { return m_kind == PT_ARRAY && m_array_variadic; }
		bool isMutablePointer() const { return m_kind == PT_POINTER && m_pointer_mutable; }

		const PType& getArrayElement() const;
		const PType& getPointerElement() const;
		const PType& getOptionalElement() const;
		const std::vector<PType>& getTypeList() const;

		bool isNamed() const;
		const interp::QualifiedId& name() const;

		static PType named(interp::QualifiedId qid);
		static PType named(const char* name);

		static PType function(std::vector<PType> params, PType ret);
		static PType array(PType elm, bool variadic = false);
		static PType variadicArray(PType elm);
		static PType pointer(PType elm, bool is_mut);
		static PType optional(PType elm);

		std::string str() const;

		~PType();

		bool operator==(const PType& a) const = default;
		bool operator!=(const PType& a) const = default;

	private:
		PType(interp::QualifiedId name, int kind, std::vector<PType> type_list);

		static constexpr int PT_FUNCTION = 1;
		static constexpr int PT_POINTER = 2;
		static constexpr int PT_ARRAY = 3;
		static constexpr int PT_OPTIONAL = 4;

		int m_kind;
		interp::QualifiedId m_name;
		std::vector<PType> m_type_list;
		bool m_array_variadic;
		bool m_pointer_mutable;
	};
}
