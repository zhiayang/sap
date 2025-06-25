// assign_op.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
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
				util::unreachable();
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
