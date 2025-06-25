// hook.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> HookBlock::evaluate_impl(Evaluator* ev) const
	{
		if(ev->interpreter()->currentPhase() != this->phase)
			return EvalResult::ofVoid();

		return this->body->evaluate(ev);
	}
}
