// variable.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"         // for VariableDefn, Expr, Declaration, Var...
#include "interp/type.h"        // for Type
#include "interp/interp.h"      // for Interpreter, DefnTree, StackFrame
#include "interp/eval_result.h" // for EvalResult, TRY_VALUE

namespace sap::interp::cst
{
	ErrorOr<EvalResult> VariableDefn::evaluate_impl(Evaluator* ev) const
	{
		if(this->initialiser != nullptr)
		{
			// if this is a global variable that was already initialised, don't re-initialise it
			// the implication is that for layout passes > 1, we don't reset values (we need to persist some state!)
			if(this->is_global && ev->getGlobalValue(this) != nullptr)
				return EvalResult::ofVoid();

			auto value = ev->castValue(TRY_VALUE(this->initialiser->evaluate(ev)), this->declaration->type);
			if(this->is_global)
				ev->setGlobalValue(this, std::move(value));
			else
				ev->frame().setValue(this, std::move(value));
		}

		return EvalResult::ofVoid();
	}
}
