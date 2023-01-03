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
	ErrorOr<TCResult> FunctionDecl::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		std::vector<const Type*> param_types {};
		for(auto& param : m_params)
			param_types.push_back(TRY(cs->resolveType(param.type)));

		TRY(cs->current()->declare(this));
		return TCResult::ofRValue(Type::makeFunction(std::move(param_types), TRY(cs->resolveType(m_return_type))));
	}

	ErrorOr<TCResult> BuiltinFunctionDefn::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return this->declaration->typecheck(cs);
	}


	ErrorOr<TCResult> FunctionDefn::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		this->declaration->resolved_defn = this;
		auto decl_type = TRY(this->declaration->typecheck(cs)).type();

		// TODO: maybe a less weird mangling solution? idk
		auto tree = cs->current()->lookupOrDeclareNamespace(zpr::sprint("{}${}", this->declaration->name, decl_type->str()));
		auto _ = cs->pushTree(tree);

		// make definitions for our parameters
		for(auto& param : static_cast<FunctionDecl*>(this->declaration.get())->params())
		{
			auto resolved_type = TRY(cs->resolveType(param.type));
			if(param.default_value != nullptr)
			{
				auto ty = TRY(param.default_value->typecheck(cs)).type();
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

		auto return_type = decl_type->toFunction()->returnType();
		auto __ = cs->enterFunctionWithReturnType(return_type);

		TRY(this->body->typecheck(cs));

		return TCResult::ofRValue(decl_type);
	}

	ErrorOr<EvalResult> FunctionDefn::call(Interpreter* cs, std::vector<Value>& args) const
	{
		auto _ = cs->pushFrame();
		auto& frame = cs->frame();

		if(args.size() != this->param_defns.size())
			return ErrFmt("function call arity mismatch");

		for(size_t i = 0; i < args.size(); i++)
			frame.setValue(this->param_defns[i].get(), std::move(args[i]));

		return this->body->evaluate(cs);
	}


	ErrorOr<TCResult> ReturnStmt::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		if(not cs->isCurrentlyInFunction())
			return ErrFmt("invalid use of 'return' outside of a function body");

		auto ret_type = cs->getCurrentFunctionReturnType();
		this->return_value_type = ret_type;

		auto expr_type = Type::makeVoid();
		if(this->expr != nullptr)
			expr_type = TRY(this->expr->typecheck(cs, ret_type)).type();

		if(not cs->canImplicitlyConvert(expr_type, ret_type))
			return ErrFmt("cannot return value of type '{}' in function returning '{}'", expr_type, ret_type);

		return TCResult::ofVoid();
	}

	ErrorOr<EvalResult> ReturnStmt::evaluate(Interpreter* cs) const
	{
		if(this->expr == nullptr)
			return EvalResult::ofReturnVoid();

		auto value = TRY_VALUE(this->expr->evaluate(cs));
		value = cs->castValue(std::move(value), this->return_value_type);

		return EvalResult::ofReturnValue(std::move(value));
	}










	// evaluating these don't do anything
	ErrorOr<EvalResult> FunctionDefn::evaluate(Interpreter* cs) const
	{
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> FunctionDecl::evaluate(Interpreter* cs) const
	{
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> BuiltinFunctionDefn::evaluate(Interpreter* cs) const
	{
		return EvalResult::ofVoid();
	}
}
