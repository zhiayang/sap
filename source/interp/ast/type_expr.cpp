// type_expr.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
	ErrorOr<EvalResult> TypeExpr::evaluate_impl(Evaluator* ev) const
	{
		return ErrMsg(ev, "type expressions cannot be used in evaluated contexts");
		return EvalResult::ofVoid();
	}

	ErrorOr<TCResult> TypeExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto t = TRY(ts->resolveType(this->ptype));
		return TCResult::ofRValue(t);
	}

	ErrorOr<TCResult2> TypeExpr::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto t = TRY(ts->resolveType(this->ptype));
		return TCResult2::ofRValue<cst::TypeExpr>(m_location, t);
	}
}
