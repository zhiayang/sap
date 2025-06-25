// unary_op.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "util.h"

#include "interp/cst.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> UnaryOp::evaluate_impl(Evaluator* ev) const
	{
		auto val = TRY_VALUE(this->expr->evaluate(ev));
		switch(this->op)
		{
			case Op::Plus: {
				return EvalResult::ofValue(std::move(val));
			}

			case Op::Minus: {
				if(val.type()->isInteger())
					return EvalResult::ofValue(Value::integer(-val.getInteger()));
				else if(val.type()->isFloating())
					return EvalResult::ofValue(Value::floating(-val.getFloating()));
				else if(val.type()->isLength())
					return EvalResult::ofValue(Value::length(val.getLength().negate()));
				else
					assert(false);
			}

			case Op::LogicalNot: {
				return EvalResult::ofValue(Value::boolean(not val.getBool()));
			}
		}
		util::unreachable();
	}
}
