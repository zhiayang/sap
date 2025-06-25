// return.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
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

			bool found = false;
			while(frame != nullptr && frame->callDepth() == cur_call_depth)
			{
				if(frame->containsValue(*value.lValuePointer()))
				{
					ret = ev->castValue(value.move(), this->type);
					found = true;
					break;
				}

				frame = frame->parent();
			}

			// if we didn't find it, then it's a global of some kind;
			// don't try to move it out, just return it.
			if(not found)
				ret = value.take();
		}
		else
		{
			ret = ev->castValue(value.take(), this->type);
		}

		return EvalResult::ofReturnValue(std::move(ret));
	}
}
