// misc.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> OptionalCheckOp::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		auto inside = TRY(this->expr->typecheck(ts, infer));
		if(not inside.type()->isOptional())
			return ErrFmt("invalid use of '?' on non-optional type '{}'", inside.type());

		return TCResult::ofRValue(Type::makeBool());
	}

	ErrorOr<EvalResult> OptionalCheckOp::evaluate(Evaluator* ev) const
	{
		auto inside = TRY_VALUE(this->expr->evaluate(ev));
		assert(inside.isOptional());

		auto tmp = std::move(inside).takeOptional();
		return EvalResult::ofValue(Value::boolean(tmp.has_value()));
	}
}
