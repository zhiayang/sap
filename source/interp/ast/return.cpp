// return.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> ReturnStmt::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		if(not ts->isCurrentlyInFunction())
			return ErrMsg(ts, "invalid use of 'return' outside of a function body");

		auto ret_type = ts->getCurrentFunctionReturnType();

		std::unique_ptr<cst::Expr> return_value {};
		if(this->expr != nullptr)
			return_value = TRY(this->expr->typecheck(ts, ret_type, /* move: */ true)).take_expr();

		auto expr_type = return_value ? return_value->type() : Type::makeVoid();
		if(not ts->canImplicitlyConvert(expr_type, ret_type))
			return ErrMsg(ts, "cannot return value of type '{}' in function returning '{}'", expr_type, ret_type);

		return TCResult::ofVoid<cst::ReturnStmt>(m_location, ret_type, std::move(return_value));
	}
}
