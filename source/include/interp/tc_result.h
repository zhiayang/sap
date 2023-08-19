// tc_result.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "interp/type.h"

namespace sap::interp
{
	namespace ast
	{
		struct Expr;
		struct Declaration;
	}

	struct TCResult
	{
		const Type* type() const { return m_type; }
		bool isMutable() const { return m_is_mutable; }
		bool isLValue() const { return m_is_lvalue; }

		bool isGeneric() const { return m_generic_decl != nullptr; }
		const ast::Declaration* genericDecl() const { return m_generic_decl; }

		TCResult replacingType(const Type* type) const { return TCResult(type, m_is_mutable, m_is_lvalue); }

		static ErrorOr<TCResult> ofVoid() { return Ok(TCResult(Type::makeVoid(), false, false)); }
		static ErrorOr<TCResult> ofRValue(const Type* type) { return Ok(TCResult(type, false, false)); }
		static ErrorOr<TCResult> ofLValue(const Type* type, bool is_mutable)
		{
			return Ok(TCResult(type, is_mutable, true));
		}

		static ErrorOr<TCResult> ofGeneric(const ast::Declaration* decl, util::hashmap<std::string, ast::Expr*> args)
		{
			assert(decl != nullptr);
			return Ok(TCResult(Type::makeVoid(), false, false, decl, std::move(args)));
		}

	private:
		explicit TCResult(const Type* type,
		    bool is_mutable,
		    bool is_lvalue,
		    const ast::Declaration* generic_decl = nullptr,
		    util::hashmap<std::string, ast::Expr*> generic_args = {})
		    : m_type(type)
		    , m_is_mutable(is_mutable)
		    , m_is_lvalue(is_lvalue)
		    , m_generic_decl(generic_decl)
		    , m_generic_args(std::move(generic_args))
		{
		}

		const Type* m_type;
		bool m_is_mutable;
		bool m_is_lvalue;

		const ast::Declaration* m_generic_decl;
		util::hashmap<std::string, ast::Expr*> m_generic_args;
	};
}
