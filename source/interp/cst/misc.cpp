// misc.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> ArraySpreadOp::evaluate_impl(Evaluator* ev) const
	{
		auto arr = TRY_VALUE(this->expr->evaluate(ev)).takeArray();
		return EvalResult::ofValue(Value::array(m_type->arrayElement(), std::move(arr),
		    /* variadic: */ true));
	}
}
