// if.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp
{
	ErrorOr<TCResult> IfStmt::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		auto cond = TRY(this->if_cond->typecheck(cs, Type::makeBool()));
		if(not cond.type()->isBool())
			return ErrFmt("if condition requires boolean expression");

		TRY(this->if_body->typecheck(cs));
		if(this->else_body)
			TRY(this->else_body->typecheck(cs));

		return TCResult::ofVoid();
	}

	ErrorOr<EvalResult> IfStmt::evaluate(Interpreter* cs) const
	{
		auto cond = TRY_VALUE(this->if_cond->evaluate(cs));
		cond = cs->castValue(std::move(cond), Type::makeBool());

		if(cond.getBool())
			TRY(this->if_body->evaluate(cs));
		else if(this->else_body != nullptr)
			TRY(this->else_body->evaluate(cs));

		return EvalResult::ofVoid();
	}
}
