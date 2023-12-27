// if_let.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> IfLetOptionalStmt::evaluate_impl(Evaluator* ev) const
	{
		auto f = ev->pushFrame();

		assert(not this->defn->is_global);
		TRY(this->defn->evaluate(ev));

		auto value = ev->frame().valueOf(this->defn.get());
		assert(value != nullptr);
		assert(value->type()->isOptional());

		if(value->haveOptionalValue())
		{
			if(auto ret = TRY(this->true_case->evaluate(ev)); not ret.isNormal())
				return OkMove(ret);
		}
		else if(this->else_case != nullptr)
		{
			if(auto ret = TRY(this->else_case->evaluate(ev)); not ret.isNormal())
				return OkMove(ret);
		}

		return EvalResult::ofVoid();
	}


	ErrorOr<EvalResult> IfLetUnionStmt::evaluate_impl(Evaluator* ev) const
	{
		auto f = ev->pushFrame();

		assert(not this->rhs_defn->is_global);
		TRY(this->rhs_defn->evaluate(ev));

		auto value = ev->frame().valueOf(this->rhs_defn.get());
		assert(value != nullptr);

		if(value->getUnionVariantIndex() == this->variant_index)
		{
			auto f_ = ev->pushFrame();
			for(auto& d : this->binding_defns)
				TRY(d->evaluate(ev));

			if(auto ret = TRY(this->true_case->evaluate(ev)); not ret.isNormal())
				return OkMove(ret);
		}
		else if(this->else_case != nullptr)
		{
			if(auto ret = TRY(this->else_case->evaluate(ev)); not ret.isNormal())
				return OkMove(ret);
		}

		return EvalResult::ofVoid();
	}
}
