// cast.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> CastExpr::evaluate_impl(Evaluator* ev) const
	{
		auto val = TRY_VALUE(this->expr->evaluate(ev));

		Value result {};
		switch(this->cast_kind)
		{
			using enum CastKind;

			case None: return ErrMsg(ev, "invalid cast!");

			case Implicit: //
				result = ev->castValue(std::move(val), m_type);
				break;

			case Pointer: //
				if(m_type->isMutablePointer())
					result = Value::mutablePointer(m_type->pointerElement(), val.getMutablePointer());
				else
					result = Value::pointer(m_type->pointerElement(), val.getPointer());
				break;

			case FloatToInteger: //
				result = Value::integer(static_cast<int64_t>(val.getFloating()));
				break;

			case CharToInteger: //
				result = Value::integer(static_cast<int64_t>(val.getChar()));
				break;

			case IntegerToChar: //
				result = Value::character(static_cast<char32_t>(val.getInteger()));
				break;
		}

		return EvalResult::ofValue(std::move(result));
	}

	ErrorOr<EvalResult> UnionVariantCastExpr::evaluate_impl(Evaluator* ev) const
	{
		auto val = TRY(this->expr->evaluate(ev));
		if(auto uvi = val.get().getUnionVariantIndex(); uvi != this->variant_idx)
		{
			return ErrMsg(ev, "value was variant '{}', cannot convert to '{}'", this->union_type->getCases()[uvi].name,
			    this->union_type->getCases()[this->variant_idx].name);
		}

		if(val.isLValue())
			return EvalResult::ofLValue(val.lValuePointer()->getUnionUnderlyingStruct());
		else
			return EvalResult::ofValue(val.take().takeUnionUnderlyingStruct());
	}
}
