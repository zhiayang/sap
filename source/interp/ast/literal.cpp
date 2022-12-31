// literal.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"

namespace sap::interp
{
	ErrorOr<TCResult> NumberLit::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		if(this->is_floating)
			return TCResult::ofRValue(Type::makeFloating());
		else
			return TCResult::ofRValue(Type::makeInteger());
	}

	ErrorOr<EvalResult> NumberLit::evaluate(Interpreter* cs) const
	{
		if(this->is_floating)
			return EvalResult::ofValue(Value::floating(float_value));
		else
			return EvalResult::ofValue(Value::integer(int_value));
	}

	ErrorOr<TCResult> StringLit::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return TCResult::ofRValue(Type::makeString());
	}

	ErrorOr<EvalResult> StringLit::evaluate(Interpreter* cs) const
	{
		return EvalResult::ofValue(Value::string(this->string));
	}
}
