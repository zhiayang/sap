// move.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> MoveExpr::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		auto ret = TRY(this->expr->typecheck(ts, infer));
		if(not ret.isLValue())
			return ErrMsg(ts, "invalid use of move-expression on rvalue");

		return TCResult::ofRValue(ret.type());
	}

	ErrorOr<EvalResult> MoveExpr::evaluate_impl(Evaluator* ev) const
	{
		auto inside = TRY(this->expr->evaluate(ev));
		if(not inside.isLValue())
			return ErrMsg(ev, "expected lvalue");

		return EvalResult::ofValue(inside.move());
	}
}
