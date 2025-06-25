// unary_op.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "util.h"

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
	const char* op_to_string(UnaryOp::Op op)
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

	static cst::UnaryOp::Op ast_op_to_cst_op(UnaryOp::Op op)
	{
		switch(op)
		{
			using enum UnaryOp::Op;
			case Plus: return cst::UnaryOp::Op::Plus;
			case Minus: return cst::UnaryOp::Op::Minus;
			case LogicalNot: return cst::UnaryOp::Op::LogicalNot;
		}
		util::unreachable();
	}

	ErrorOr<TCResult> UnaryOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		switch(this->op)
		{
			case Op::Plus:
			case Op::Minus: {
				auto inside = TRY(this->expr->typecheck(ts));
				auto ty = inside.type();

				if(not(ty->isInteger() || ty->isFloating() || ty->isLength()))
				{
					return ErrMsg(this->expr->loc(), "invalid type '{}' for unary '{}' operator", ty,
					    op_to_string(this->op));
				}

				// same type as we got in
				return TCResult::ofRValue<cst::UnaryOp>(m_location, ty, ast_op_to_cst_op(this->op),
				    std::move(inside).take_expr());
			}

			case Op::LogicalNot: {
				auto inside = TRY(this->expr->typecheck(ts, /* infer: */ Type::makeBool()));
				auto ty = inside.type();

				if(not ty->isBool())
					return ErrMsg(this->expr->loc(), "invalid type '{}' for unary logical not operator", ty);

				return TCResult::ofRValue<cst::UnaryOp>(m_location, Type::makeBool(), ast_op_to_cst_op(this->op),
				    std::move(inside).take_expr());
			}
		}
		util::unreachable();
	}
}
