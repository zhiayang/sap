// binary_op.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for is_one_of

#include "interp/cst.h"         // for BinaryOp::Op, BinaryOp, Expr, Binary...
#include "interp/type.h"        // for Type, ArrayType
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/eval_result.h" // for EvalResult, TRY_VALUE

namespace sap::interp::cst
{
	ErrorOr<Value> evaluateBinaryOperationOnValues(Evaluator* ev, BinaryOp::Op op, Value lval, Value rval)
	{
		using Op = BinaryOp::Op;

		auto ltype = lval.type();
		auto rtype = rval.type();

		auto do_arith = [](Op oper, auto a, auto b) {
			switch(oper)
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
			}
			util::unreachable();
		};


		if((ltype->isInteger() && rtype->isInteger()) || (ltype->isFloating() && rtype->isFloating()))
		{
			if(util::is_one_of(op, Op::Add, Op::Subtract, Op::Multiply, Op::Divide, Op::Modulo))
			{
				// TODO: this might be bad because implicit conversions
				if(ltype->isInteger())
					return Ok(Value::integer(do_arith(op, lval.getInteger(), rval.getInteger())));
				else
					return Ok(Value::floating(do_arith(op, lval.getFloating(), rval.getFloating())));
			}
		}
		else if(ltype->isArray() && rtype->isArray() && ltype->arrayElement() == rtype->arrayElement())
		{
			if(op == Op::Add)
			{
				auto lhs = std::move(lval).takeArray();
				auto rhs = std::move(rval).takeArray();

				for(auto& x : rhs)
					lhs.push_back(std::move(x));

				return Ok(Value::array(ltype->arrayElement(), std::move(lhs)));
			}
		}
		else if((ltype->isArray() && rtype->isInteger()) || (ltype->isInteger() && rtype->isArray()))
		{
			if(op == Op::Multiply)
			{
				auto elm = (ltype->isArray() ? ltype : rtype)->arrayElement();
				auto arr = ltype->isArray() ? std::move(lval) : std::move(rval);
				auto num = (ltype->isArray() ? std::move(rval) : std::move(lval)).getInteger();

				if(num <= 0)
					return Ok(Value::array(elm, {}));

				std::vector<Value> ret = std::move(arr.clone()).takeArray();

				for(int64_t i = 1; i < num; i++)
				{
					auto copy = std::move(arr.clone()).takeArray();
					for(auto& c : copy)
						ret.push_back(std::move(c));
				}

				return Ok(Value::array(elm, std::move(ret)));
			}
		}
		else if(ltype->isLength() && rtype->isLength())
		{
			assert(op == Op::Add || op == Op::Subtract);

			auto style = ev->currentStyle();

			auto left = lval.getLength().resolve(style.font(), style.font_size(), style.root_font_size());
			auto right = rval.getLength().resolve(style.font(), style.font_size(), style.root_font_size());

			sap::Length result = 0;
			if(op == Op::Add)
				result = left + right;
			else
				result = left - right;

			return Ok(Value::length(DynLength(result)));
		}
		else if((ltype->isFloating() || ltype->isInteger()) && rtype->isLength())
		{
			assert(op == Op::Multiply);
			auto len = rval.getLength();

			double multiplier = 0;
			if(lval.isFloating())
				multiplier = lval.getFloating();
			else
				multiplier = static_cast<double>(lval.getInteger());

			return Ok(Value::length(DynLength(len.value() * multiplier, len.unit())));
		}
		else if(ltype->isLength() && (rtype->isFloating() || rtype->isInteger()))
		{
			assert(op == Op::Multiply || op == Op::Divide);
			auto len = lval.getLength();

			double multiplier = 0;
			if(rval.isFloating())
				multiplier = rval.getFloating();
			else
				multiplier = static_cast<double>(rval.getInteger());

			if(op == Op::Divide)
				multiplier = 1.0 / multiplier;

			return Ok(Value::length(DynLength(len.value() * multiplier, len.unit())));
		}

		assert(false && "unreachable!");
	}

	ErrorOr<EvalResult> BinaryOp::evaluate_impl(Evaluator* ev) const
	{
		auto lval = TRY_VALUE(this->lhs->evaluate(ev));
		auto rval = TRY_VALUE(this->rhs->evaluate(ev));

		return EvalResult::ofValue(TRY(evaluateBinaryOperationOnValues(ev, //
		    this->op, std::move(lval), std::move(rval))));
	}
}
