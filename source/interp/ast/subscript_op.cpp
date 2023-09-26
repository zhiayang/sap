// subscript_op.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
#if 0
	ErrorOr<TCResult> SubscriptOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lhs = TRY(this->array->typecheck(ts));

		// TODO: multiple subscript indices for overloaded operators
		auto rhs = TRY(this->indices[0]->typecheck(ts));

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
#endif
	ErrorOr<TCResult2> SubscriptOp::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lhs = TRY(this->array->typecheck2(ts));

		// TODO: multiple subscript indices for overloaded operators
		auto rhs = TRY(this->indices[0]->typecheck2(ts));

		auto ltype = lhs.type();
		auto rtype = rhs.type();
		if(not ltype->isArray())
			return ErrMsg(ts, "invalid use of operator '[]' on non-array type '{}'", ltype);
		if(not rtype->isInteger())
			return ErrMsg(ts, "subscript index must be an integer");

		std::vector<std::unique_ptr<cst::Expr>> new_indices {};
		new_indices.push_back(std::move(rhs).take_expr());

		// FIXME: lots of code dupe
		if(lhs.isLValue())
		{
			if(lhs.isMutable())
			{
				return TCResult2::ofMutableLValue<cst::SubscriptOp>(m_location, ltype->arrayElement(),
				    std::move(lhs).take_expr(), std::move(new_indices));
			}
			else
			{
				return TCResult2::ofImmutableLValue<cst::SubscriptOp>(m_location, ltype->arrayElement(),
				    std::move(lhs).take_expr(), std::move(new_indices));
			}
		}
		else
		{
			return TCResult2::ofRValue<cst::SubscriptOp>(m_location, ltype->arrayElement(), std::move(lhs).take_expr(),
			    std::move(new_indices));
		}
	}

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
