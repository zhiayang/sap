// ident.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/type.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> Ident::evaluate_impl(Evaluator* ev) const
	{
		// this should have been set by typechecking!
		assert(this->resolved_decl != nullptr);

		auto* defn = this->resolved_decl->definition();
		if(defn->declaration->function_decl.has_value())
		{
			if(not defn->declaration->type->isFunction())
				return ErrMsg(ev, "expected function type, found '{}'", defn->declaration->type);

			return EvalResult::ofValue(Value::function(defn->declaration->type->toFunction(),
			    [defn](Interpreter* cs, std::vector<Value>& args) -> std::optional<Value> { //
				    auto ret = cs->evaluator().call(defn, args);
				    if(ret.is_err())
					    sap::internal_error("failed to evaluate function");

				    if(ret->hasValue())
					    return ret.unwrap().take();
				    else
					    return std::nullopt;
			    }));
		}

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
