// if.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp
{
	StrErrorOr<TCResult> IfStmt::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		auto cond = TRY(this->if_cond->typecheck(ts, Type::makeBool()));
		if(not cond.type()->isBool())
			return ErrFmt("cannot convert '{}' to boolean condition", cond.type());

		TRY(this->if_body->typecheck(ts));
		if(this->else_body)
			TRY(this->else_body->typecheck(ts));

		return TCResult::ofVoid();
	}

	StrErrorOr<EvalResult> IfStmt::evaluate(Evaluator* ev) const
	{
		auto cond = TRY_VALUE(this->if_cond->evaluate(ev));
		cond = ev->castValue(std::move(cond), Type::makeBool());

		if(cond.getBool())
		{
			if(auto ret = TRY(this->if_body->evaluate(ev)); not ret.isNormal())
				return Ok(std::move(ret));
		}
		else if(this->else_body != nullptr)
		{
			if(auto ret = TRY(this->else_body->evaluate(ev)); not ret.isNormal())
				return Ok(std::move(ret));
		}

		return EvalResult::ofVoid();
	}
}
