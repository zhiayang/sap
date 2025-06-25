// if_let.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> IfLetOptionalStmt::evaluate_impl(Evaluator* ev) const
	{
		auto f = ev->pushFrame();
		auto value = TRY_VALUE(this->expr->evaluate(ev));
		assert(value.type()->isOptional());

		if(value.haveOptionalValue())
		{
			TRY(this->defn->evaluate_impl(ev));
			ev->frame().setValue(this->defn.get(), std::move(value).takeOptional().value());

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
		auto value = TRY_VALUE(this->expr->evaluate(ev));

		assert(not this->rhs_defn->is_global);
		TRY(this->rhs_defn->evaluate(ev));

		if(value.getUnionVariantIndex() == this->variant_index)
		{
			auto def = this->rhs_defn.get();

			auto& variant = value.getUnionUnderlyingStruct();
			if(def->is_mutable)
				ev->frame().setValue(def, Value::mutablePointer(variant.type(), &variant));
			else
				ev->frame().setValue(def, Value::pointer(variant.type(), &variant));

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
