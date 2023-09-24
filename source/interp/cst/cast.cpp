// cast.cpp
// Copyright (c) 2022, zhiayang
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
}
