// logical_op.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for is_one_of

#include "interp/cst.h"         // for BinaryOp::Op, BinaryOp, Expr, Binary...
#include "interp/type.h"        // for Type, ArrayType
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/eval_result.h" // for EvalResult, TRY_VALUE

namespace sap::interp::cst
{
	ErrorOr<EvalResult> LogicalBinOp::evaluate_impl(Evaluator* ev) const
	{
		auto lval = TRY_VALUE(this->lhs->evaluate(ev));
		if(not lval.isBool())
			return ErrMsg(ev, "expected bool");

		if(this->op == Op::And && lval.getBool() == false)
			return EvalResult::ofValue(Value::boolean(false));

		else if(this->op == Op::Or && lval.getBool() == true)
			return EvalResult::ofValue(Value::boolean(true));

		auto rval = TRY_VALUE(this->rhs->evaluate(ev));
		if(not rval.isBool())
			return ErrMsg(ev, "expected bool");

		return EvalResult::ofValue(std::move(rval));
	}
}
