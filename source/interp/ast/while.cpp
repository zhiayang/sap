// while.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> WhileLoop::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto cond = TRY(this->condition->typecheck(ts, Type::makeBool()));
		if(not cond.type()->isBool())
			return ErrMsg(ts, "cannot convert '{}' to boolean condition", cond.type());

		auto _ = ts->enterLoopBody();
		auto new_body = TRY(this->body->typecheck(ts)).take<cst::Block>();

		return TCResult::ofVoid<cst::WhileLoop>(m_location, std::move(cond).take_expr(), std::move(new_body));
	}

	ErrorOr<TCResult> BreakStmt::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		if(not ts->isCurrentlyInLoopBody())
			return ErrMsg(ts, "invalid use of 'break' outside of a loop body");

		return TCResult::ofVoid<cst::BreakStmt>(m_location);
	}

	ErrorOr<TCResult> ContinueStmt::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		if(not ts->isCurrentlyInLoopBody())
			return ErrMsg(ts, "invalid use of 'continue' outside of a loop body");

		return TCResult::ofVoid<cst::ContinueStmt>(m_location);
	}
}
