// literal.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/eval_result.h"

namespace sap::interp
{
	ErrorOr<TCResult> LengthExpr::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		return TCResult::ofRValue(Type::makeLength());
	}

	ErrorOr<EvalResult> LengthExpr::evaluate(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::length(this->length));
	}




	ErrorOr<TCResult> NumberLit::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		if(this->is_floating)
			return TCResult::ofRValue(Type::makeFloating());
		else
			return TCResult::ofRValue(Type::makeInteger());
	}

	ErrorOr<EvalResult> NumberLit::evaluate(Evaluator* ev) const
	{
		if(this->is_floating)
			return EvalResult::ofValue(Value::floating(float_value));
		else
			return EvalResult::ofValue(Value::integer(int_value));
	}

	ErrorOr<TCResult> StringLit::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		return TCResult::ofRValue(Type::makeString());
	}

	ErrorOr<EvalResult> StringLit::evaluate(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::string(this->string));
	}



	ErrorOr<TCResult> NullLit::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		return TCResult::ofRValue(Type::makeNullPtr());
	}

	ErrorOr<EvalResult> NullLit::evaluate(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::nullPointer());
	}
}
