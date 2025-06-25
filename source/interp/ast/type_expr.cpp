// type_expr.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> TypeExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto t = TRY(ts->resolveType(this->ptype));
		return TCResult::ofRValue<cst::TypeExpr>(m_location, t);
	}
}
