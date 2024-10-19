// deref.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> DereferenceOp::evaluate_impl(Evaluator* ev) const
	{
		auto expr_res = TRY(this->expr->evaluate(ev));
		assert(expr_res.hasValue());

		auto& inside = expr_res.get();
		auto type = inside.type();

		if(type->isOptional())
		{
			auto opt = inside.getOptional();
			if(opt.has_value())
			{
				if(expr_res.isLValue())
					return EvalResult::ofValue((*opt)->clone());
				else
					return EvalResult::ofValue(std::move(**opt));
			}
			else
			{
				return ErrMsg(ev, "dereferencing empty optional");
			}
		}
		else
		{
			auto ptr = inside.getPointer();
			if(ptr == nullptr)
				return ErrMsg(ev, "dereferencing null pointer");

			return EvalResult::ofLValue(const_cast<Value&>(*ptr));
		}
	}





	ErrorOr<EvalResult> AddressOfOp::evaluate_impl(Evaluator* ev) const
	{
		auto expr_res = TRY(this->expr->evaluate(ev));
		assert(expr_res.hasValue());
		assert(expr_res.isLValue());
		assert(m_type->isPointer());

		auto& inside = expr_res.get();
		// if(auto inside_ty = inside.type();
		//     inside_ty->isTreeBlockObj() || inside_ty->isTreeInlineObj() || inside_ty->isLayoutObject())
		// {
		// 	return EvalResult::ofVoid();
		// }
		// else
		{
			Value ret {};
			if(this->is_mutable)
				ret = Value::mutablePointer(m_type->pointerElement(), &inside);
			else
				ret = Value::pointer(m_type->pointerElement(), &inside);

			return EvalResult::ofValue(std::move(ret));
		}
	}
}
