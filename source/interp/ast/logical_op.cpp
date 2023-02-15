// logical_op.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for is_one_of

#include "interp/ast.h"         // for BinaryOp::Op, BinaryOp, Expr, Binary...
#include "interp/type.h"        // for Type, ArrayType
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/eval_result.h" // for EvalResult, TRY_VALUE

namespace sap::interp
{
	static const char* op_to_string(LogicalBinOp::Op op)
	{
		switch(op)
		{
			using enum LogicalBinOp::Op;
			case And: return "and";
			case Or: return "or";
		}
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
