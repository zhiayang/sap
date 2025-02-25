// func.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"         // for FunctionDefn, VariableDefn, Function...
#include "interp/type.h"        // for Type, FunctionType
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter, DefnTree, StackFrame
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp::cst
{
	ErrorOr<EvalResult> FunctionDefn::call(Evaluator* ev, std::vector<Value>& args) const
	{
		auto _ = ev->pushFrame();
		auto& frame = ev->frame();

		if(args.size() != this->params.size())
			return ErrMsg(ev, "function call arity mismatch");

		for(size_t i = 0; i < args.size(); i++)
			frame.setValue(this->params[i].defn.get(), std::move(args[i]));

		return this->body->evaluate(ev);
	}

	// evaluating these don't do anything
	ErrorOr<EvalResult> FunctionDefn::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> BuiltinFunctionDefn::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> PartiallyResolvedOverloadSet::evaluate_impl(Evaluator* ev) const
	{
		if(this->items.empty())
			return ErrMsg(this->loc(), "reference to overload set '{}' resulted in an empty set", this->name);

		auto e = ErrorMessage(this->loc(), zpr::sprint("ambiguous reference to '{}' in overload set", this->name));
		for(auto& item : this->items)
			e.addInfo(item.decl->location, zpr::sprint("possible candidate:"));

		return Err(std::move(e));
	}
}
