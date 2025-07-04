// block.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> Block::evaluate_impl(Evaluator* ev) const
	{
		auto _ = ev->pushFrame();

		for(auto& stmt : this->body)
		{
			auto result = TRY(stmt->evaluate(ev));
			ev->frame().dropTemporaries();

			if(not result.isNormal())
				return OkMove(result);
		}

		return EvalResult::ofVoid();
	}
}
