// if.cpp
// Copyright (c) 2022, zhiayang
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

		TRY(this->if_body->typecheck(ts));
		if(this->else_body)
			TRY(this->else_body->typecheck(ts));

		return TCResult::ofVoid();
	}

	ErrorOr<TCResult2> IfStmt::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto cond = TRY(this->if_cond->typecheck2(ts, Type::makeBool()));
		if(not cond.type()->isBool())
			return ErrMsg(ts, "cannot convert '{}' to boolean condition", cond.type());

		auto new_if = std::make_unique<cst::IfStmt>(m_location);
		new_if->if_cond = std::move(cond).take_expr();
		new_if->if_body = TRY(this->if_body->typecheck2(ts)).take<cst::Block>();

		if(this->else_body)
			new_if->else_body = TRY(this->else_body->typecheck2(ts)).take<cst::Block>();

		return TCResult2::ofVoid(std::move(new_if));
	}


	ErrorOr<EvalResult> IfStmt::evaluate_impl(Evaluator* ev) const
	{
		auto cond = TRY_VALUE(this->if_cond->evaluate(ev));
		cond = ev->castValue(std::move(cond), Type::makeBool());

		if(cond.getBool())
		{
			if(auto ret = TRY(this->if_body->evaluate(ev)); not ret.isNormal())
				return Ok(std::move(ret));
		}
		else if(this->else_body != nullptr)
		{
			if(auto ret = TRY(this->else_body->evaluate(ev)); not ret.isNormal())
				return Ok(std::move(ret));
		}

		return EvalResult::ofVoid();
	}
}
