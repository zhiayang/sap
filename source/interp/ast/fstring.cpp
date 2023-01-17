// fstring.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> FStringExpr::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		return ErrMsg(ts, "sadge");
	}


	ErrorOr<EvalResult> FStringExpr::evaluate_impl(Evaluator* ev) const
	{
		return ErrMsg(ev, "sadge");
	}
}
