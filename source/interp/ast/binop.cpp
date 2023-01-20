// binop.cpp
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
	static const char* op_to_string(BinaryOp::Op op)
	{
		switch(op)
		{
			case BinaryOp::Op::Add: return "+";
			case BinaryOp::Op::Subtract: return "-";
			case BinaryOp::Op::Multiply: return "*";
			case BinaryOp::Op::Divide: return "/";
			case BinaryOp::Op::Modulo: return "%";
		}
	}

	ErrorOr<TCResult> BinaryOp::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		auto ltype = TRY(this->lhs->typecheck(ts)).type();
		auto rtype = TRY(this->rhs->typecheck(ts)).type();

		if((ltype->isInteger() && rtype->isInteger()) || (ltype->isFloating() && rtype->isFloating()))
		{
			if(util::is_one_of(this->op, Op::Add, Op::Subtract, Op::Multiply, Op::Divide, Op::Modulo))
				return TCResult::ofRValue(ltype);
		}
		else if(ltype->isArray() && rtype->isArray() && ltype->arrayElement() == rtype->arrayElement())
		{
			if(this->op == Op::Add)
				return TCResult::ofRValue(ltype);
		}
		else if((ltype->isArray() && rtype->isInteger()) || (ltype->isInteger() && rtype->isArray()))
		{
			if(this->op == Op::Multiply)
				return TCResult::ofRValue(ltype->isArray() ? ltype : rtype);
		}

		return ErrMsg(ts, "unsupported operation '{}' between types '{}' and '{}'", op_to_string(this->op), ltype, rtype);
	}

	ErrorOr<EvalResult> BinaryOp::evaluate_impl(Evaluator* ev) const
	{
		auto lval = TRY_VALUE(this->lhs->evaluate(ev));
		auto ltype = lval.type();

		auto rval = TRY_VALUE(this->rhs->evaluate(ev));
		auto rtype = rval.type();

		auto do_arith = [](Op op, auto a, auto b) {
			switch(op)
			{
				case Op::Add: return a + b;
				case Op::Subtract: return a - b;
				case Op::Multiply: return a * b;
				case Op::Divide: return a / b;
				case Op::Modulo: {
					if constexpr(std::is_floating_point_v<decltype(a)>)
						return fmod(a, b);
					else
						return a % b;
				}
				default: assert(false && "unreachable!");
			}
		};


		if((ltype->isInteger() && rtype->isInteger()) || (ltype->isFloating() && rtype->isFloating()))
		{
			if(util::is_one_of(this->op, Op::Add, Op::Subtract, Op::Multiply, Op::Divide, Op::Modulo))
			{
				// TODO: this might be bad because implicit conversions
				if(ltype->isInteger())
					return EvalResult::ofValue(Value::integer(do_arith(this->op, lval.getInteger(), rval.getInteger())));
				else
					return EvalResult::ofValue(Value::floating(do_arith(this->op, lval.getFloating(), rval.getFloating())));
			}
		}
		else if(ltype->isArray() && rtype->isArray() && ltype->arrayElement() == rtype->arrayElement())
		{
			if(this->op == Op::Add)
			{
				auto lhs = std::move(lval).takeArray();
				auto rhs = std::move(rval).takeArray();

				for(auto& x : rhs)
					lhs.push_back(std::move(x));

				return EvalResult::ofValue(Value::array(ltype->arrayElement(), std::move(lhs)));
			}
		}
		else if((ltype->isArray() && rtype->isInteger()) || (ltype->isInteger() && rtype->isArray()))
		{
			if(this->op == Op::Multiply)
			{
				auto elm = (ltype->isArray() ? ltype : rtype)->arrayElement();
				auto arr = ltype->isArray() ? std::move(lval) : std::move(rval);
				auto num = (ltype->isArray() ? std::move(rval) : std::move(lval)).getInteger();

				if(num <= 0)
					return EvalResult::ofValue(Value::array(elm, {}));

				std::vector<Value> ret = std::move(arr.clone()).takeArray();

				for(int64_t i = 1; i < num; i++)
				{
					auto copy = std::move(arr.clone()).takeArray();
					for(auto& c : copy)
						ret.push_back(std::move(c));
				}

				return EvalResult::ofValue(Value::array(elm, std::move(ret)));
			}
		}

		assert(false && "unreachable!");
	}
}
