// binary_op.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for is_one_of

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
	const char* op_to_string(BinaryOp::Op op)
	{
		switch(op)
		{
			case BinaryOp::Op::Add: return "+";
			case BinaryOp::Op::Subtract: return "-";
			case BinaryOp::Op::Multiply: return "*";
			case BinaryOp::Op::Divide: return "/";
			case BinaryOp::Op::Modulo: return "%";
		}
		util::unreachable();
	}

	static cst::BinaryOp::Op ast_op_to_cst_op(BinaryOp::Op op)
	{
		switch(op)
		{
			case BinaryOp::Op::Add: return cst::BinaryOp::Op::Add;
			case BinaryOp::Op::Subtract: return cst::BinaryOp::Op::Subtract;
			case BinaryOp::Op::Multiply: return cst::BinaryOp::Op::Multiply;
			case BinaryOp::Op::Divide: return cst::BinaryOp::Op::Divide;
			case BinaryOp::Op::Modulo: return cst::BinaryOp::Op::Modulo;
		}
		util::unreachable();
	}

	ErrorOr<TCResult> BinaryOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lres = TRY(this->lhs->typecheck(ts));
		auto rres = TRY(this->rhs->typecheck(ts));

		auto ltype = lres.type();
		auto rtype = rres.type();

		auto make_result = [&](const Type* type) -> auto {
			return TCResult::ofRValue(std::make_unique<cst::BinaryOp>(m_location, type, //
			    ast_op_to_cst_op(this->op),                                             //
			    std::move(lres).take_expr(),                                            //
			    std::move(rres).take_expr()));
		};

		if((ltype->isInteger() && rtype->isInteger()) || (ltype->isFloating() && rtype->isFloating()))
		{
			if(util::is_one_of(this->op, Op::Add, Op::Subtract, Op::Multiply, Op::Divide, Op::Modulo))
				return make_result(ltype);
		}
		else if(ltype->isArray() && rtype->isArray() && ltype->arrayElement() == rtype->arrayElement())
		{
			if(this->op == Op::Add)
				return make_result(ltype);
		}
		else if((ltype->isArray() && rtype->isInteger()) || (ltype->isInteger() && rtype->isArray()))
		{
			if(this->op == Op::Multiply)
			{
				auto arr_type = ltype->isArray() ? ltype : rtype;
				if(not arr_type->isCloneable())
				{
					return ErrMsg(ts, "cannot copy array of type '{}' because array element type cannot be copied",
					    arr_type);
				}

				return make_result(ltype->isArray() ? ltype : rtype);
			}
		}
		else if(ltype->isLength() && rtype->isLength())
		{
			if(this->op == Op::Add || this->op == Op::Subtract)
				return make_result(Type::makeLength());
		}
		else if((ltype->isFloating() || ltype->isInteger()) && rtype->isLength())
		{
			if(this->op == Op::Multiply)
				return make_result(Type::makeLength());
		}
		else if(ltype->isLength() && (rtype->isFloating() || rtype->isInteger()))
		{
			if(this->op == Op::Multiply || this->op == Op::Divide)
				return make_result(Type::makeLength());
		}

		return ErrMsg(ts, "unsupported operation '{}' between types '{}' and '{}'", op_to_string(this->op), ltype,
		    rtype);
	}
}
