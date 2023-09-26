// ident.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"         // for Ident, Declaration
#include "interp/type.h"        // for Type
#include "interp/interp.h"      // for Interpreter, StackFrame, DefnTree
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp::cst
{
	ErrorOr<EvalResult> Ident::evaluate_impl(Evaluator* ev) const
	{
		// this should have been set by typechecking!
		assert(this->resolved_decl != nullptr);

		auto* defn = this->resolved_decl->definition();

		auto* frame = &ev->frame();
		while(frame != nullptr)
		{
			if(auto value = frame->valueOf(defn); value != nullptr)
				return EvalResult::ofLValue(*value);

			frame = frame->parent();
		}

		// we didn't find any locals -- check globals.
		if(auto value = ev->getGlobalValue(defn); value != nullptr)
			return EvalResult::ofLValue(*value);

		return ErrMsg(ev, "use of uninitialised variable '{}'", this->name);
	}
}
