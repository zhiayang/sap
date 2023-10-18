// enum.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> EnumeratorDefn::evaluate_impl(Evaluator* ev) const
	{
		auto et = this->declaration->type->toEnum();
		if(this->value == nullptr)
		{
			assert(et->elementType()->isInteger());

			int64_t prev_value = 0;
			if(this->prev_sibling != nullptr)
				prev_value = ev->getGlobalValue(this->prev_sibling)->getEnumerator().getInteger();

			ev->setGlobalValue(this, Value::enumerator(et, Value::integer(prev_value)));
		}
		else
		{
			auto tmp = TRY_VALUE(this->value->evaluate(ev));
			auto val = Value::enumerator(et, ev->castValue(std::move(tmp), et->elementType()));

			ev->setGlobalValue(this, std::move(val));
		}

		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> EnumDefn::evaluate_impl(Evaluator* ev) const
	{
		// this does nothing (all the work is done by the enumerators)
		for(auto& e : this->enumerators)
			TRY(e->evaluate(ev));

		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> EnumeratorExpr::evaluate_impl(Evaluator* ev) const
	{
		auto v = ev->getGlobalValue(this->enumerator);
		assert(v != nullptr);

		return EvalResult::ofLValue(*v);
	}
}
