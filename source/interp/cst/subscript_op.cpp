// subscript_op.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> SubscriptOp::evaluate_impl(Evaluator* ev) const
	{
		auto lhs_res = TRY(this->array->evaluate(ev));
		auto rhs = TRY_VALUE(this->indices[0]->evaluate(ev));

		auto idx = checked_cast<size_t>(rhs.getInteger());

		if(lhs_res.isLValue())
		{
			auto& arr = lhs_res.lValuePointer()->getArray();
			if(idx >= arr.size())
				return ErrMsg(ev, "array index out of bounds (idx={}, size={})", idx, arr.size());

			auto& val = arr[idx];
			return EvalResult::ofLValue(*const_cast<Value*>(&val));
		}
		else
		{
			auto arr = lhs_res.take().takeArray();
			if(idx >= arr.size())
				return ErrMsg(ev, "array index out of bounds (idx={}, size={})", idx, arr.size());

			return EvalResult::ofValue(std::move(arr[idx]));
		}
	}
}
