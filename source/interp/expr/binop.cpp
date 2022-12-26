// binop.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/type.h"
#include "interp/state.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/basedefs.h"

namespace sap::interp
{
	static const char* op_to_string(Op op)
	{
		switch(op)
		{
			case Op::Add:
				return "+";
			case Op::Subtract:
				return "-";
			case Op::Multiply:
				return "*";
			case Op::Divide:
				return "/";
		}
	}

	template <typename T, std::same_as<T>... Ts>
	static constexpr bool one_of(T foo, Ts... foos)
	{
		return (false || ... || (foo == foos));
	}


	ErrorOr<const Type*> BinaryOp::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		auto ltype = TRY(this->lhs->typecheck(cs));
		auto rtype = TRY(this->rhs->typecheck(cs));

		if((ltype->isInteger() && rtype->isInteger()) || (ltype->isFloating() && rtype->isFloating()))
		{
			if(one_of(this->op, Op::Add, Op::Subtract, Op::Multiply, Op::Divide))
				return Ok(ltype);
		}
		else if(ltype->isArray() && rtype->isArray() && ltype->toArray()->elementType() == rtype->toArray()->elementType())
		{
			if(this->op == Op::Add)
				return Ok(ltype);
		}
		else if((ltype->isArray() && rtype->isInteger()) || (ltype->isInteger() && rtype->isArray()))
		{
			if(this->op == Op::Multiply)
				return Ok(ltype->isArray() ? ltype : rtype);
		}

		return ErrFmt("unsupported operation '{}' between types '{}' and '{}'", op_to_string(this->op), ltype, rtype);
	}

	ErrorOr<std::optional<Value>> BinaryOp::evaluate(Interpreter* cs) const
	{
		// TODO: maybe check for empty optional here
		auto lval = *TRY(this->lhs->evaluate(cs));
		auto ltype = lval.type();

		auto rval = *TRY(this->rhs->evaluate(cs));
		auto rtype = rval.type();

		auto do_op = [](Op op, auto a, auto b) {
			switch(op)
			{
				case Op::Add:
					return a + b;
				case Op::Subtract:
					return a - b;
				case Op::Multiply:
					return a * b;
				case Op::Divide:
					return a / b;
			}
		};

		if((ltype->isInteger() && rtype->isInteger()) || (ltype->isFloating() && rtype->isFloating()))
		{
			if(one_of(this->op, Op::Add, Op::Subtract, Op::Multiply, Op::Divide))
			{
				// TODO: this might be bad because implicit conversions
				if(ltype->isInteger())
					return Ok(Value::integer(do_op(this->op, lval.getInteger(), rval.getInteger())));
				else
					return Ok(Value::floating(do_op(this->op, lval.getFloating(), rval.getFloating())));
			}
		}
		else if(ltype->isArray() && rtype->isArray() && ltype->toArray()->elementType() == rtype->toArray()->elementType())
		{
			if(this->op == Op::Add)
			{
				auto lhs = std::move(lval).takeArray();
				auto rhs = std::move(rval).takeArray();

				for(auto& x : rhs)
					lhs.push_back(std::move(x));

				return Ok(Value::array(ltype->toArray()->elementType(), std::move(lhs)));
			}
		}
		else if((ltype->isArray() && rtype->isInteger()) || (ltype->isInteger() && rtype->isArray()))
		{
			if(this->op == Op::Multiply)
			{
				auto elm = (ltype->isArray() ? ltype : rtype)->toArray()->elementType();
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

		assert(false && "unreachable!");
	}
}
