// misc.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> ArraySpreadOp::evaluate_impl(Evaluator* ev) const
	{
		auto arr = TRY_VALUE(this->expr->evaluate(ev)).takeArray();
		return EvalResult::ofValue(Value::array(m_type->arrayElement(), std::move(arr),
		    /* variadic: */ true));
	}

	ErrorOr<EvalResult> VariadicPackExpr::evaluate_impl(Evaluator* ev) const
	{
		auto elm_type = m_type->arrayElement();

		std::vector<Value> values {};
		for(auto& e : this->exprs)
		{
			auto val = TRY_VALUE(e->evaluate(ev));

			// need to flatten, because otherwise ...[a] will end up as [[a]].
			if(val.type()->isVariadicArray())
			{
				auto arr = std::move(val).takeArray();
				for(auto& v : arr)
					values.push_back(ev->castValue(std::move(v), elm_type));
			}
			else
			{
				values.push_back(ev->castValue(std::move(val), elm_type));
			}
		}

		return EvalResult::ofValue(Value::array(elm_type, std::move(values),
		    /* variadic: */ true));
	}


	ErrorOr<EvalResult> TypeExpr::evaluate_impl(Evaluator* ev) const
	{
		return ErrMsg(ev, "oh no");
	}
}
