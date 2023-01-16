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
	ErrorOr<TCResult> FunctionDecl::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		std::vector<const Type*> param_types {};
		for(auto& param : m_params)
			param_types.push_back(TRY(ts->resolveType(param.type)));

		auto fn_type = Type::makeFunction(std::move(param_types), TRY(ts->resolveType(m_return_type)));
		m_tc_result = TCResult::ofRValue(fn_type).unwrap();

		TRY(ts->current()->declare(this));
		return Ok(*m_tc_result);
	}

	ErrorOr<TCResult> BuiltinFunctionDefn::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		return this->declaration->typecheck(ts);
	}


	ErrorOr<TCResult> FunctionDefn::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		this->declaration->resolve(this);
		auto decl_type = TRY(this->declaration->typecheck(ts)).type();

		// TODO: maybe a less weird mangling solution? idk
		auto tree = ts->current()->lookupOrDeclareNamespace(zpr::sprint("{}${}", this->declaration->name, decl_type->str()));
		auto _ = ts->pushTree(tree);

		// make definitions for our parameters
		for(auto& param : static_cast<FunctionDecl*>(this->declaration.get())->params())
		{
			auto resolved_type = TRY(ts->resolveType(param.type));
			if(param.default_value != nullptr)
			{
				auto ty = TRY(param.default_value->typecheck(ts)).type();
				if(not ts->canImplicitlyConvert(ty, resolved_type))
				{
					return ErrFmt("default value for parameter '{}' has type '{}', "
					              "which is incompatible with parameter type '{}'",
					    param.name, resolved_type, ty);
				}
			}

			this->param_defns.push_back(std::make_unique<VariableDefn>(param.name, //
			    /* mutable: */ false, /* init: */ nullptr, param.type));

			TRY(this->param_defns.back()->typecheck(ts));
		}

		auto return_type = decl_type->toFunction()->returnType();
		auto __ = ts->enterFunctionWithReturnType(return_type);

		TRY(this->body->typecheck(ts));

		if(not return_type->isVoid())
		{
			if(not this->body->checkAllPathsReturn(return_type))
				return ErrFmt("not all control paths return a value");
		}

		return TCResult::ofRValue(decl_type);
	}

	ErrorOr<EvalResult> FunctionDefn::call(Evaluator* ev, std::vector<Value>& args) const
	{
		auto _ = ev->pushFrame();
		auto& frame = ev->frame();

		if(args.size() != this->param_defns.size())
			return ErrFmt("function call arity mismatch");

		for(size_t i = 0; i < args.size(); i++)
			frame.setValue(this->param_defns[i].get(), std::move(args[i]));

		return this->body->evaluate(ev);
	}


	ErrorOr<TCResult> ReturnStmt::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		if(not ts->isCurrentlyInFunction())
			return ErrFmt("invalid use of 'return' outside of a function body");

		auto ret_type = ts->getCurrentFunctionReturnType();
		this->return_value_type = ret_type;

		auto expr_type = Type::makeVoid();
		if(this->expr != nullptr)
			expr_type = TRY(this->expr->typecheck(ts, ret_type)).type();

		if(not ts->canImplicitlyConvert(expr_type, ret_type))
			return ErrFmt("cannot return value of type '{}' in function returning '{}'", expr_type, ret_type);

		return TCResult::ofVoid();
	}

	ErrorOr<EvalResult> ReturnStmt::evaluate(Evaluator* ev) const
	{
		if(this->expr == nullptr)
			return EvalResult::ofReturnVoid();

		auto value = TRY_VALUE(this->expr->evaluate(ev));
		value = ev->castValue(std::move(value), this->return_value_type);

		return EvalResult::ofReturnValue(std::move(value));
	}










	// evaluating these don't do anything
	ErrorOr<EvalResult> FunctionDefn::evaluate(Evaluator* ev) const
	{
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> FunctionDecl::evaluate(Evaluator* ev) const
	{
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> BuiltinFunctionDefn::evaluate(Evaluator* ev) const
	{
		return EvalResult::ofVoid();
	}
}
