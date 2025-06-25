// misc.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> ArraySpreadOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto value = TRY(this->expr->typecheck(ts, infer));
		if(not value.type()->isArray())
			return ErrMsg(ts, "invalid use of '...' operator on non-array type '{}'", value.type());

		auto ret_type = Type::makeArray(value.type()->arrayElement(), /* variadic: */ true);

		if(value.isLValue() && not ret_type->arrayElement()->isCloneable())
		{
			return ErrMsg(ts, "arrays of type '{}' cannot be spread with `...` without first moving; use `...*`",
			    ret_type->arrayElement());
		}

		return TCResult::ofRValue<cst::ArraySpreadOp>(m_location, ret_type, std::move(value).take_expr());
	}
}
