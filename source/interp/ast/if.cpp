// if.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> IfStmt::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto cond = TRY(this->if_cond->typecheck(ts, Type::makeBool()));
		if(not cond.type()->isBool())
			return ErrMsg(ts, "cannot convert '{}' to boolean condition", cond.type());

		auto new_if = std::make_unique<cst::IfStmt>(m_location);
		new_if->if_cond = std::move(cond).take_expr();
		new_if->if_body = TRY(this->true_case->typecheck(ts)).take<cst::Block>();

		if(this->else_case)
			new_if->else_body = TRY(this->else_case->typecheck(ts)).take<cst::Block>();

		return TCResult::ofVoid(std::move(new_if));
	}
}
