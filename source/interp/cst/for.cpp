// for.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> ForLoop::evaluate_impl(Evaluator* ev) const
	{
		auto _ = ev->pushFrame();
		if(this->init != nullptr)
			TRY(this->init->evaluate(ev));

		bool loop_cond = true;
		while(loop_cond)
		{
			if(this->cond != nullptr)
			{
				auto cond_val = TRY_VALUE(this->cond->evaluate(ev));
				loop_cond = ev->castValue(std::move(cond_val), Type::makeBool()).getBool();
				if(not loop_cond)
					break;
			}

			auto val = TRY(this->body->evaluate(ev));
			if(not val.isNormal())
			{
				if(val.isReturn())
					return Ok(std::move(val));
				else if(val.isLoopBreak())
					break;
				else if(val.isLoopContinue())
					(void) false; // do nothing
			}

			if(this->update != nullptr)
				TRY(this->update->evaluate(ev));
		}

		return EvalResult::ofVoid();
	}
}
