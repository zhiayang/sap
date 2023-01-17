// return.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> ReturnStmt::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		if(not ts->isCurrentlyInFunction())
			return ErrMsg(ts, "invalid use of 'return' outside of a function body");

		auto ret_type = ts->getCurrentFunctionReturnType();
		this->return_value_type = ret_type;

		auto expr_type = Type::makeVoid();
		if(this->expr != nullptr)
			expr_type = TRY(this->expr->typecheck(ts, ret_type)).type();

		if(not ts->canImplicitlyConvert(expr_type, ret_type))
			return ErrMsg(ts, "cannot return value of type '{}' in function returning '{}'", expr_type, ret_type);

		return TCResult::ofVoid();
	}

	ErrorOr<EvalResult> ReturnStmt::evaluate_impl(Evaluator* ev) const
	{
		if(this->expr == nullptr)
			return EvalResult::ofReturnVoid();

		auto value = TRY(this->expr->evaluate(ev));
		Value ret {};

		if(value.isLValue())
		{
			// check whether the thing exists in the current call frame
			auto cur_call_depth = ev->frame().callDepth();
			auto frame = &ev->frame();
			while(frame != nullptr && frame->callDepth() == cur_call_depth)
			{
				if(frame->containsValue(*value.lValuePointer()))
				{
					ret = ev->castValue(value.move(), this->return_value_type);
					break;
				}

				frame = ev->frame().parent();
			}
		}
		else
		{
			ret = ev->castValue(value.take(), this->return_value_type);
		}

		return EvalResult::ofReturnValue(std::move(ret));
	}
}
