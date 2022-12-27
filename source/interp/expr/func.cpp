// func.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"    // for VariableDefn, FunctionDefn, FunctionDecl
#include "interp/type.h"   // for Type, FunctionType
#include "interp/value.h"  // for Value
#include "interp/interp.h" // for Interpreter, DefnTree

namespace sap::interp
{
	ErrorOr<const Type*> FunctionDecl::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		std::vector<const Type*> param_types {};
		for(auto& param : m_params)
			param_types.push_back(param.type);

		TRY(cs->current()->declare(this));
		return Ok(Type::makeFunction(std::move(param_types), m_return_type));
	}

	ErrorOr<const Type*> BuiltinFunctionDefn::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return this->declaration->typecheck(cs);
	}

	ErrorOr<const Type*> Block::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		auto tree = cs->current()->declareAnonymousNamespace();
		auto _ = cs->pushTree(tree);

		for(auto& stmt : this->body)
			TRY(stmt->typecheck(cs));

		return Ok(Type::makeVoid());
	}


	ErrorOr<const Type*> FunctionDefn::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		auto decl_type = TRY(this->declaration->typecheck(cs));

		// TODO: maybe a less weird mangling solution? idk
		auto tree = cs->current()->lookupOrDeclareNamespace(zpr::sprint("{}${}", this->declaration->name, decl_type->str()));
		auto _ = cs->pushTree(tree);

		// make definitions for our parameters
		for(auto& param : static_cast<FunctionDecl*>(this->declaration.get())->params())
		{
			if(param.default_value != nullptr)
			{
				auto ty = TRY(param.default_value->typecheck(cs));
				if(not cs->canImplicitlyConvert(ty, param.type))
				{
					return ErrFmt("default value for parameter '{}' has type '{}', "
					              "which is incompatible with parameter type '{}'",
					    param.name, param.type, ty);
				}
			}

			this->param_defns.push_back(std::make_unique<VariableDefn>(param.name, //
			    /* mutable: */ false, /* init: */ nullptr, param.type));
			TRY(this->param_defns.back()->typecheck(cs));
		}

		for(auto& stmt : this->body)
			TRY(stmt->typecheck(cs));

		return Ok(decl_type);
	}


	// evaluating these don't do anything
	ErrorOr<std::optional<Value>> FunctionDefn::evaluate(Interpreter* cs) const
	{
		return Ok(std::nullopt);
	}

	ErrorOr<std::optional<Value>> FunctionDecl::evaluate(Interpreter* cs) const
	{
		return Ok(std::nullopt);
	}

	ErrorOr<std::optional<Value>> BuiltinFunctionDefn::evaluate(Interpreter* cs) const
	{
		return Ok(std::nullopt);
	}
}
