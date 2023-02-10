// misc.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> ArraySpreadOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto type = TRY(this->expr->typecheck(ts, infer)).type();
		if(not type->isArray())
			return ErrMsg(ts, "invalid use of '...' operator on non-array type '{}'", type);

		return TCResult::ofRValue(Type::makeArray(type->arrayElement(), /* variadic: */ true));
	}

	ErrorOr<EvalResult> ArraySpreadOp::evaluate_impl(Evaluator* ev) const
	{
		auto arr = TRY_VALUE(this->expr->evaluate(ev)).takeArray();
		return EvalResult::ofValue(Value::array(this->get_type()->arrayElement(), std::move(arr), /* variadic: */ true));
	}
}
