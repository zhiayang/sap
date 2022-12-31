// literal.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"

namespace sap::interp
{
	ErrorOr<const Type*> NumberLit::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		if(this->is_floating)
			return Ok(Type::makeFloating());
		else
			return Ok(Type::makeInteger());
	}

	ErrorOr<EvalResult> NumberLit::evaluate(Interpreter* cs) const
	{
		if(this->is_floating)
			return EvalResult::of_value(Value::floating(float_value));
		else
			return EvalResult::of_value(Value::integer(int_value));
	}

	ErrorOr<const Type*> StringLit::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return Ok(Type::makeString());
	}

	ErrorOr<EvalResult> StringLit::evaluate(Interpreter* cs) const
	{
		return EvalResult::of_value(Value::string(this->string));
	}
}
