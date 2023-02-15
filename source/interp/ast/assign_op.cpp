// assign_op.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
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

	ErrorOr<EvalResult> AssignOp::evaluate_impl(Evaluator* ev) const
	{
		auto lval_result = TRY(this->lhs->evaluate(ev));
		auto ltype = lval_result.get().type();

		if(not lval_result.isLValue())
			return ErrMsg(ev, "cannot assign to non-lvalue");

		// auto rval =
		Value result_value {};

		if(this->op != Op::None)
		{
			auto bin_op = [this]() {
				switch(this->op)
				{
					case Op::Add: return BinaryOp::Op::Add;
					case Op::Subtract: return BinaryOp::Op::Subtract;
					case Op::Multiply: return BinaryOp::Op::Multiply;
					case Op::Divide: return BinaryOp::Op::Divide;
					case Op::Modulo: return BinaryOp::Op::Modulo;
					case Op::None: assert(false && "unreachable!");
				}
			}();

			auto rval = TRY_VALUE(this->rhs->evaluate(ev));
			result_value = TRY(evaluateBinaryOperationOnValues(ev, bin_op, std::move(lval_result.get()),
				std::move(rval)));
		}
		else
		{
			result_value = TRY_VALUE(this->rhs->evaluate(ev));
		}

		// TODO: 'any' might need work here
		auto value = ev->castValue(std::move(result_value), ltype);
		if(value.type() != ltype)
			return ErrMsg(ev, "cannot assign to '{}' from incompatible type '{}'", ltype, value.type());

		lval_result.get() = std::move(value);
		return EvalResult::ofVoid();
	}
}
