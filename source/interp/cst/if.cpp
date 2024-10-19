// if.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> IfStmt::evaluate_impl(Evaluator* ev) const
	{
		auto cond = TRY_VALUE(this->if_cond->evaluate(ev));
		cond = ev->castValue(std::move(cond), Type::makeBool());

		if(cond.getBool())
		{
			if(auto ret = TRY(this->if_body->evaluate(ev)); not ret.isNormal())
				return OkMove(ret);
		}
		else if(this->else_body != nullptr)
		{
			if(auto ret = TRY(this->else_body->evaluate(ev)); not ret.isNormal())
				return OkMove(ret);
		}

		return EvalResult::ofVoid();
	}
}
