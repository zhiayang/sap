// assign_op.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	static const char* op_to_string(AssignOp::Op op)
	{
		switch(op)
		{
			case AssignOp::Op::None: return "=";
			case AssignOp::Op::Add: return "+=";
			case AssignOp::Op::Subtract: return "-=";
			case AssignOp::Op::Multiply: return "*=";
			case AssignOp::Op::Divide: return "/=";
			case AssignOp::Op::Modulo: return "%=";
		}
		util::unreachable();
	}

	static cst::AssignOp::Op ast_op_to_cst_op(AssignOp::Op op)
	{
		switch(op)
		{
			case AssignOp::Op::None: return cst::AssignOp::Op::None;
			case AssignOp::Op::Add: return cst::AssignOp::Op::Add;
			case AssignOp::Op::Subtract: return cst::AssignOp::Op::Subtract;
			case AssignOp::Op::Multiply: return cst::AssignOp::Op::Multiply;
			case AssignOp::Op::Divide: return cst::AssignOp::Op::Divide;
			case AssignOp::Op::Modulo: return cst::AssignOp::Op::Modulo;
		}
		util::unreachable();
	}

	ErrorOr<TCResult> AssignOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lres = TRY(this->lhs->typecheck(ts, /* infer: */ nullptr, /* keep_lvalue: */ true));
		if(not lres.isLValue())
			return ErrMsg(ts, "cannot assign to non-lvalue");
		else if(not lres.isMutable())
			return ErrMsg(ts, "cannot assign to immutable lvalue");

		auto ltype = lres.type();
		auto rtype = TRY(this->rhs->typecheck(ts)).type();

		if((ltype->isInteger() && rtype->isInteger()) || (ltype->isFloating() && rtype->isFloating()))
		{
			if(util::is_one_of(this->op, Op::None, Op::Add, Op::Subtract, Op::Multiply, Op::Divide, Op::Modulo))
				return TCResult::ofVoid();
		}
		else if(ltype->isArray() && rtype->isArray() && ltype->arrayElement() == rtype->arrayElement())
		{
			if(this->op == Op::Add || this->op == Op::None)
				return TCResult::ofVoid();
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

				return TCResult::ofVoid();
			}
		}
		else if(this->op == Op::None && ts->canImplicitlyConvert(rtype, ltype))
		{
			return TCResult::ofVoid();
		}
		else if(ltype->isLength() && rtype->isLength())
		{
			if(this->op == Op::Add || this->op == Op::Subtract)
				return TCResult::ofRValue(Type::makeLength());
		}
		else if((ltype->isFloating() || ltype->isInteger()) && rtype->isLength())
		{
			if(this->op == Op::Multiply)
				return TCResult::ofRValue(Type::makeLength());
		}
		else if(ltype->isLength() && (rtype->isFloating() || rtype->isInteger()))
		{
			if(this->op == Op::Multiply || this->op == Op::Divide)
				return TCResult::ofRValue(Type::makeLength());
		}

		if(this->op == Op::None)
			return ErrMsg(ts, "cannot assign to '{}' from incompatible type '{}'", ltype, rtype);

		return ErrMsg(ts, "unsupported operation '{}' between types '{}' and '{}'", op_to_string(this->op), ltype,
		    rtype);
	}

	// defined in binop.cpp
	extern ErrorOr<Value> evaluateBinaryOperationOnValues(Evaluator* ev, BinaryOp::Op op, Value lval, Value rval);

	ErrorOr<TCResult2> AssignOp::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lres = TRY(this->lhs->typecheck2(ts, /* infer: */ nullptr, /* keep_lvalue: */ true));
		if(not lres.isLValue())
			return ErrMsg(ts, "cannot assign to non-lvalue");
		else if(not lres.isMutable())
			return ErrMsg(ts, "cannot assign to immutable lvalue");

		auto ltype = lres.type();

		auto rres = TRY(this->rhs->typecheck2(ts));
		auto rtype = rres.type();

		auto cst_op = std::make_unique<cst::AssignOp>(m_location, ast_op_to_cst_op(this->op),
		    std::move(lres).take_expr(), std::move(rres).take_expr());

		if((ltype->isInteger() && rtype->isInteger()) || (ltype->isFloating() && rtype->isFloating()))
		{
			if(util::is_one_of(this->op, Op::None, Op::Add, Op::Subtract, Op::Multiply, Op::Divide, Op::Modulo))
				goto pass;
		}
		else if(ltype->isArray() && rtype->isArray() && ltype->arrayElement() == rtype->arrayElement())
		{
			if(this->op == Op::Add || this->op == Op::None)
				goto pass;
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

				goto pass;
			}
		}
		else if(this->op == Op::None && ts->canImplicitlyConvert(rtype, ltype))
		{
			goto pass;
		}
		else if(ltype->isLength() && rtype->isLength())
		{
			if(this->op == Op::Add || this->op == Op::Subtract)
				goto pass;
		}
		else if((ltype->isFloating() || ltype->isInteger()) && rtype->isLength())
		{
			if(this->op == Op::Multiply)
				goto pass;
		}
		else if(ltype->isLength() && (rtype->isFloating() || rtype->isInteger()))
		{
			if(this->op == Op::Multiply || this->op == Op::Divide)
				goto pass;
		}

		if(this->op == Op::None)
			return ErrMsg(ts, "cannot assign to '{}' from incompatible type '{}'", ltype, rtype);

		return ErrMsg(ts, "unsupported operation '{}' between types '{}' and '{}'", op_to_string(this->op), ltype,
		    rtype);

	pass:
		return TCResult2::ofVoid(std::move(cst_op));
	}
}
