// move.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> MoveExpr::evaluate_impl(Evaluator* ev) const
	{
		auto inside = TRY(this->expr->evaluate(ev));
		if(not inside.isLValue())
			return ErrMsg(ev, "expected lvalue");

		return EvalResult::ofValue(inside.move());
	}
}
