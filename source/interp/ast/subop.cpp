// subop.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> SubscriptOp::typecheck_impl(Typechecker* ts, const Type* infer, bool moving) const
	{
		auto lhs = TRY(this->array->typecheck(ts));
		auto rhs = TRY(this->index->typecheck(ts));

		auto ltype = lhs.type();
		auto rtype = rhs.type();
		if(not ltype->isArray())
			return ErrMsg(ts, "invalid use of operator '[]' on non-array type '{}'", ltype);
		if(not rtype->isInteger())
			return ErrMsg(ts, "subscript index must be an integer");

		if(lhs.isLValue())
			return TCResult::ofLValue(ltype->arrayElement(), lhs.isMutable());
		else
			return TCResult::ofRValue(ltype->arrayElement());
	}

	ErrorOr<EvalResult> SubscriptOp::evaluate_impl(Evaluator* ev) const
	{
		auto lhs_res = TRY(this->array->evaluate(ev));
		auto rhs = TRY_VALUE(this->index->evaluate(ev));

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
