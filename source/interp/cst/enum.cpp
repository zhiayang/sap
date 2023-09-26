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
			assert(this->prev_sibling != nullptr);

			auto prev_value = ev->getGlobalValue(this->prev_sibling)->getInteger();
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
}
