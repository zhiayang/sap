// move.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> MoveExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto ret = TRY(this->expr->typecheck(ts, infer, /* keep_lvalue: */ true));
		if(not ret.isLValue())
			return ErrMsg(ts, "invalid use of move-expression on rvalue");

		return TCResult::ofRValue<cst::MoveExpr>(m_location, std::move(ret).take_expr());
	}
}
