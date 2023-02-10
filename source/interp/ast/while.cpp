// while.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> WhileLoop::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto cond = TRY(this->condition->typecheck(ts, Type::makeBool()));
		if(not cond.type()->isBool())
			return ErrMsg(ts, "cannot convert '{}' to boolean condition", cond.type());

		auto _ = ts->enterLoopBody();
		TRY(this->body->typecheck(ts));

		return TCResult::ofVoid();
	}

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
					return Ok(std::move(val));
				else if(val.isLoopBreak())
					break;
				else if(val.isLoopContinue())
					continue;
			}
		} while(loop_cond);

		return EvalResult::ofVoid();
	}














	ErrorOr<TCResult> BreakStmt::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		if(not ts->isCurrentlyInLoopBody())
			return ErrMsg(ts, "invalid use of 'break' outside of a loop body");

		return TCResult::ofVoid();
	}

	ErrorOr<EvalResult> BreakStmt::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofLoopBreak();
	}

	ErrorOr<TCResult> ContinueStmt::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		if(not ts->isCurrentlyInLoopBody())
			return ErrMsg(ts, "invalid use of 'continue' outside of a loop body");

		return TCResult::ofVoid();
	}

	ErrorOr<EvalResult> ContinueStmt::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofLoopContinue();
	}
}
