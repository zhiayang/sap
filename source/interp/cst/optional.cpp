// optional.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> NullCoalesceOp::evaluate_impl(Evaluator* ev) const
	{
		// short circuit -- don't evaluate rhs eagerly
		auto lval = TRY_VALUE(this->lhs->evaluate(ev));
		auto ltype = lval.type();

		assert(lval.isOptional() || lval.isNull() || lval.type()->isPointer());
		bool left_has_value = not lval.isNull()
		                   && ((lval.isOptional() && lval.haveOptionalValue()) //
		                       || (lval.isPointer() && lval.getPointer() != nullptr));

		if(left_has_value)
		{
			// if it's the same, then just return it. this means that both the left and right
			// sides were either pointers or optionals.
			if(this->kind == Kind::Flatmap)
				return EvalResult::ofValue(ev->castValue(std::move(lval), m_type));

			// otherwise, the right type is the "element" of the left type
			if(ltype->isOptional())
			{
				auto ret = ev->castValue(std::move(lval).takeOptional().value(), m_type);
				return EvalResult::ofValue(std::move(ret));
			}
			else
			{
				auto ret = ev->castValue(lval.getPointer()->clone(), m_type);
				return EvalResult::ofValue(std::move(ret));
			}
		}
		else
		{
			// just evaluate the right-hand side.
			return EvalResult::ofValue(TRY_VALUE(this->rhs->evaluate(ev)));
		}
	}
}
