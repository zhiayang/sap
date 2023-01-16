// move.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	StrErrorOr<TCResult> MoveExpr::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		auto ret = TRY(this->expr->typecheck(ts, infer));
		if(not ret.isLValue())
			return ErrFmt("invalid use of move-expression on rvalue");

		return TCResult::ofRValue(ret.type());
	}

	StrErrorOr<EvalResult> MoveExpr::evaluate(Evaluator* ev) const
	{
		auto inside = TRY(this->expr->evaluate(ev));
		if(not inside.isLValue())
			return ErrFmt("expected lvalue");

		return EvalResult::ofValue(inside.move());
	}
}
