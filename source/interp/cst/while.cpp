// while.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> WhileLoop::evaluate_impl(Evaluator* ev) const
	{
		bool loop_cond = false;
		do
		{
			auto cond = TRY_VALUE(this->condition->evaluate(ev));
			loop_cond = ev->castValue(std::move(cond), Type::makeBool()).getBool();

			if(not loop_cond)
				break;

			auto val = TRY(this->body->evaluate(ev));
			if(not val.isNormal())
			{
				if(val.isReturn())
					return OkMove(val);
				else if(val.isLoopBreak())
					break;
				else if(val.isLoopContinue())
					continue;
			}
		} while(loop_cond);

		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> BreakStmt::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofLoopBreak();
	}

	ErrorOr<EvalResult> ContinueStmt::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofLoopContinue();
	}
}
