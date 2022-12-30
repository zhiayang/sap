// func.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"         // for FunctionDefn, VariableDefn, Function...
#include "interp/type.h"        // for Type, FunctionType
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter, DefnTree, StackFrame
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp
{
	ErrorOr<const Type*> FunctionDecl::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		std::vector<const Type*> param_types {};
		for(auto& param : m_params)
			param_types.push_back(TRY(cs->resolveType(param.type)));

		TRY(cs->current()->declare(this));
		return Ok(Type::makeFunction(std::move(param_types), TRY(cs->resolveType(m_return_type))));
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
			auto resolved_type = TRY(cs->resolveType(param.type));
			if(param.default_value != nullptr)
			{
				auto ty = TRY(param.default_value->typecheck(cs));
				if(not cs->canImplicitlyConvert(ty, resolved_type))
				{
					return ErrFmt("default value for parameter '{}' has type '{}', "
					              "which is incompatible with parameter type '{}'",
					    param.name, resolved_type, ty);
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

	ErrorOr<EvalResult> FunctionDefn::call(Interpreter* cs, std::vector<Value>& args) const
	{
		auto _ = cs->pushFrame();
		auto& frame = cs->frame();

		if(args.size() != this->param_defns.size())
			return ErrFmt("function call arity mismatch");

		for(size_t i = 0; i < args.size(); i++)
			frame.setValue(this->param_defns[i].get(), std::move(args[i]));

		for(auto& stmt : this->body)
		{
			auto result = TRY(stmt->evaluate(cs));

			if(result.is_return())
				return Ok(std::move(result));

			else if(not result.is_normal())
				return ErrFmt("unexpected statement in function body: {}", result.kind);
		}

		return Ok(EvalResult::of_void());
	}







	// evaluating these don't do anything
	ErrorOr<EvalResult> FunctionDefn::evaluate(Interpreter* cs) const
	{
		return Ok(EvalResult::of_void());
	}

	ErrorOr<EvalResult> FunctionDecl::evaluate(Interpreter* cs) const
	{
		return Ok(EvalResult::of_void());
	}

	ErrorOr<EvalResult> BuiltinFunctionDefn::evaluate(Interpreter* cs) const
	{
		return Ok(EvalResult::of_void());
	}
}
