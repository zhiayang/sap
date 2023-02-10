// hook.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> HookBlock::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		if(ts->isCurrentlyInFunction())
			return ErrMsg(ts, "hook blocks can only appear the top level");

		auto ret = TRY(this->body->typecheck(ts, infer, keep_lvalue));
		ts->interpreter()->addHookBlock(this);

		return Ok(ret);
	}

	ErrorOr<EvalResult> HookBlock::evaluate_impl(Evaluator* ev) const
	{
		if(ev->interpreter()->currentPhase() != this->phase)
			return EvalResult::ofVoid();

		return this->body->evaluate(ev);
	}
}
