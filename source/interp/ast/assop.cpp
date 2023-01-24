// assop.cpp
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

	ErrorOr<TCResult> AssignOp::typecheck_impl(Typechecker* ts, const Type* infer, bool moving) const
	{
		auto lres = TRY(this->lhs->typecheck(ts));
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
				return TCResult::ofVoid();
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

		return ErrMsg(ts, "unsupported operation '{}' between types '{}' and '{}'", op_to_string(this->op), ltype, rtype);
	}

	ErrorOr<EvalResult> AssignOp::evaluate_impl(Evaluator* ev) const
	{
		auto lval_result = TRY(this->lhs->evaluate(ev));

		if(not lval_result.isLValue())
			return ErrMsg(ev, "cannot assign to non-lvalue");

		auto rval = TRY_VALUE(this->rhs->evaluate(ev));
		auto rtype = rval.type();

		auto ltype = lval_result.get().type();

		if(this->op != Op::None)
		{
			// read the value
			auto lval = lval_result.get().clone();

			auto do_op = [](Op op, auto a, auto b) {
				switch(op)
				{
					case Op::Add: return a + b;
					case Op::Subtract: return a - b;
					case Op::Multiply: return a * b;
					case Op::Divide: return a / b;
					case Op::Modulo:
						if constexpr(std::is_floating_point_v<decltype(a)>)
							return fmod(a, b);
						else
							return a % b;
					case Op::None: assert(false && "unreachable!"); abort();
				}
			};

			if((ltype->isInteger() && rtype->isInteger()) || (ltype->isFloating() && rtype->isFloating()))
			{
				assert(util::is_one_of(this->op, Op::Add, Op::Subtract, Op::Multiply, Op::Divide, Op::Modulo));

				if(ltype->isInteger())
					rval = Value::integer(do_op(this->op, lval.getInteger(), rval.getInteger()));
				else
					rval = Value::floating(do_op(this->op, lval.getFloating(), rval.getFloating()));
			}
			else if(ltype->isArray() && rtype->isArray() && ltype->arrayElement() == rtype->arrayElement())
			{
				assert(this->op == Op::Add);

				auto lhs = std::move(lval).takeArray();
				auto rhs = std::move(rval).takeArray();

				for(auto& x : rhs)
					lhs.push_back(std::move(x));

				rval = Value::array(ltype->arrayElement(), std::move(lhs));
			}
			else if((ltype->isArray() && rtype->isInteger()) || (ltype->isInteger() && rtype->isArray()))
			{
				assert(this->op == Op::Multiply);

				auto elm = (ltype->isArray() ? ltype : rtype)->arrayElement();
				auto arr = ltype->isArray() ? std::move(lval) : std::move(rval);
				auto num = (ltype->isArray() ? std::move(rval) : std::move(lval)).getInteger();

				if(num <= 0)
				{
					rval = Value::array(elm, {});
				}
				else
				{
					std::vector<Value> ret = std::move(arr.clone()).takeArray();

					for(int64_t i = 1; i < num; i++)
					{
						auto copy = std::move(arr.clone()).takeArray();
						for(auto& c : copy)
							ret.push_back(std::move(c));
					}

					rval = Value::array(elm, std::move(ret));
				}
			}
			else if(ltype->isLength() && rtype->isLength())
			{
				assert(this->op == Op::Add || this->op == Op::Subtract);

				auto style = ev->currentStyle();

				auto left = lval.getLength().resolve(style->font(), style->font_size(), style->root_font_size());
				auto right = rval.getLength().resolve(style->font(), style->font_size(), style->root_font_size());

				sap::Length result = 0;
				if(this->op == Op::Add)
					result = left + right;
				else
					result = left - right;

				rval = Value::length(DynLength(result));
			}
			else if((ltype->isFloating() || ltype->isInteger()) && rtype->isLength())
			{
				assert(this->op == Op::Multiply);
				auto len = lval.getLength();

				double multiplier = 0;
				if(lval.isFloating())
					multiplier = lval.getFloating();
				else
					multiplier = static_cast<double>(lval.getInteger());

				rval = Value::length(DynLength(len.value() * multiplier, len.unit()));
			}
			else if(ltype->isLength() && (rtype->isFloating() || rtype->isInteger()))
			{
				assert(this->op == Op::Multiply || this->op == Op::Divide);
				auto len = lval.getLength();

				double multiplier = 0;
				if(lval.isFloating())
					multiplier = lval.getFloating();
				else
					multiplier = static_cast<double>(lval.getInteger());

				if(this->op == Op::Divide)
					multiplier = 1.0 / multiplier;

				rval = Value::length(DynLength(len.value() * multiplier, len.unit()));
			}
		}

		// TODO: 'any' might need work here
		auto value = ev->castValue(std::move(rval), ltype);
		if(value.type() != ltype)
			return ErrMsg(ev, "cannot assign to '{}' from incompatible type '{}'", ltype, value.type());

		lval_result.get() = std::move(value);
		return EvalResult::ofVoid();
	}
}
