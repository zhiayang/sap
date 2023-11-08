// union.cpp
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"
#include "interp/overload_resolution.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> UnionDefn::evaluate_impl(Evaluator* ev) const
	{
		// do nothing
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> UnionExpr::evaluate_impl(Evaluator* ev) const
	{
		// do nothing
		return EvalResult::ofVoid();
	}
}
