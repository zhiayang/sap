// logical_op.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
	static const char* op_to_string(LogicalBinOp::Op op)
	{
		switch(op)
		{
			using enum LogicalBinOp::Op;
			case And: return "and";
			case Or: return "or";
		}
		util::unreachable();
	}

	static cst::LogicalBinOp::Op ast_op_to_cst_op(LogicalBinOp::Op op)
	{
		switch(op)
		{
			using enum LogicalBinOp::Op;
			case And: return cst::LogicalBinOp::Op::And;
			case Or: return cst::LogicalBinOp::Op::Or;
		}
		util::unreachable();
	}

	ErrorOr<TCResult> LogicalBinOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto ltype = TRY(this->lhs->typecheck(ts, /* infer: */ Type::makeBool())).type();
		auto rtype = TRY(this->rhs->typecheck(ts, /* infer: */ Type::makeBool())).type();

		if(not ltype->isBool())
		{
			return ErrMsg(this->lhs->loc(), "invalid type for operand of '{}': expected bool, got '{}'",
			    op_to_string(this->op), ltype);
		}
		else if(not rtype->isBool())
		{
			return ErrMsg(this->rhs->loc(), "invalid type for operand of '{}': expected bool, got '{}'",
			    op_to_string(this->op), rtype);
		}

		return TCResult::ofRValue(Type::makeBool());
	}

	ErrorOr<TCResult2> LogicalBinOp::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lres = TRY(this->lhs->typecheck2(ts, /* infer: */ Type::makeBool()));
		auto rres = TRY(this->rhs->typecheck2(ts, /* infer: */ Type::makeBool()));

		if(not lres.type()->isBool())
		{
			return ErrMsg(this->lhs->loc(), "invalid type for operand of '{}': expected bool, got '{}'",
			    op_to_string(this->op), lres.type());
		}
		else if(not rres.type()->isBool())
		{
			return ErrMsg(this->rhs->loc(), "invalid type for operand of '{}': expected bool, got '{}'",
			    op_to_string(this->op), rres.type());
		}

		return TCResult2::ofRValue<cst::LogicalBinOp>(m_location, ast_op_to_cst_op(this->op),
		    std::move(lres).take_expr(), std::move(rres).take_expr());
	}

	ErrorOr<EvalResult> LogicalBinOp::evaluate_impl(Evaluator* ev) const
	{
		auto lval = TRY_VALUE(this->lhs->evaluate(ev));
		if(not lval.isBool())
			return ErrMsg(ev, "expected bool");

		if(this->op == Op::And && lval.getBool() == false)
			return EvalResult::ofValue(Value::boolean(false));

		else if(this->op == Op::Or && lval.getBool() == true)
			return EvalResult::ofValue(Value::boolean(true));

		auto rval = TRY_VALUE(this->rhs->evaluate(ev));
		if(not rval.isBool())
			return ErrMsg(ev, "expected bool");

		return EvalResult::ofValue(std::move(rval));
	}
}
