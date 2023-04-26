// unary_op.cpp
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
	static const char* op_to_string(UnaryOp::Op op)
	{
		switch(op)
		{
			using enum UnaryOp::Op;
			case Plus: return "+";
			case Minus: return "-";
			case LogicalNot: return "not";
		}
		util::unreachable();
	}

	ErrorOr<TCResult> UnaryOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		switch(this->op)
		{
			case Op::Plus:
			case Op::Minus: {
				auto ty = TRY(this->expr->typecheck(ts)).type();
				if(not(ty->isInteger() || ty->isFloating() || ty->isLength()))
				{
					return ErrMsg(this->expr->loc(), "invalid type '{}' for unary '{}' operator", ty,
						op_to_string(this->op));
				}

				// same type as we got in
				return TCResult::ofRValue(ty);
			}

			case Op::LogicalNot: {
				auto ty = TRY(this->expr->typecheck(ts, /* infer: */ Type::makeBool())).type();
				if(not ty->isBool())
					return ErrMsg(this->expr->loc(), "invalid type '{}' for unary logical not operator", ty);

				return TCResult::ofRValue(Type::makeBool());
			}
		}
		util::unreachable();
	}

	ErrorOr<EvalResult> UnaryOp::evaluate_impl(Evaluator* ev) const
	{
		auto val = TRY_VALUE(this->expr->evaluate(ev));
		switch(this->op)
		{
			case Op::Plus: {
				return EvalResult::ofValue(std::move(val));
			}

			case Op::Minus: {
				if(val.type()->isInteger())
					return EvalResult::ofValue(Value::integer(-val.getInteger()));
				else if(val.type()->isFloating())
					return EvalResult::ofValue(Value::floating(-val.getFloating()));
				else if(val.type()->isLength())
					return EvalResult::ofValue(Value::length(val.getLength().negate()));
				else
					assert(false);
			}

			case Op::LogicalNot: {
				return EvalResult::ofValue(Value::boolean(not val.getBool()));
			}
		}
		util::unreachable();
	}
}
