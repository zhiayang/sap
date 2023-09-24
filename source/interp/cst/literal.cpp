// literal.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> ArrayLit::evaluate_impl(Evaluator* ev) const
	{
		std::vector<Value> values {};
		for(auto& elm : this->elements)
			values.push_back(TRY_VALUE(elm->evaluate(ev)));

		return EvalResult::ofValue(Value::array(m_type->toArray()->elementType(), std::move(values)));
	}

	ErrorOr<EvalResult> LengthExpr::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::length(this->length));
	}

	ErrorOr<EvalResult> BooleanLit::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::boolean(this->value));
	}

	ErrorOr<EvalResult> NumberLit::evaluate_impl(Evaluator* ev) const
	{
		if(this->is_floating)
			return EvalResult::ofValue(Value::floating(float_value));
		else
			return EvalResult::ofValue(Value::integer(int_value));
	}

	ErrorOr<EvalResult> StringLit::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::string(this->string));
	}

	ErrorOr<EvalResult> CharLit::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::character(this->character));
	}

	ErrorOr<EvalResult> NullLit::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::nullPointer());
	}
}
