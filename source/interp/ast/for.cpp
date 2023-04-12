// for.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> ForLoop::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		// everything should be in its own scope.
		auto tree = ts->current()->declareAnonymousNamespace();
		auto _ = ts->pushTree(tree);

		if(this->init != nullptr)
			TRY(this->init->typecheck(ts));

		if(this->cond != nullptr)
		{
			auto cond = TRY(this->cond->typecheck(ts, Type::makeBool()));
			if(not cond.type()->isBool())
				return ErrMsg(ts, "cannot convert '{}' to boolean condition", cond.type());
		}

		if(this->update != nullptr)
			TRY(this->update->typecheck(ts));

		{
			auto _ = ts->enterLoopBody();
			TRY(this->body->typecheck(ts));
		}

		return TCResult::ofVoid();
	}

	ErrorOr<EvalResult> ForLoop::evaluate_impl(Evaluator* ev) const
	{
		auto _ = ev->pushFrame();
		if(this->init != nullptr)
			TRY(this->init->evaluate(ev));

		bool loop_cond = true;
		while(loop_cond)
		{
			if(this->cond != nullptr)
			{
				auto cond = TRY_VALUE(this->cond->evaluate(ev));
				loop_cond = ev->castValue(std::move(cond), Type::makeBool()).getBool();
				if(not loop_cond)
					break;
			}

			auto val = TRY(this->body->evaluate(ev));
			if(not val.isNormal())
			{
				if(val.isReturn())
					return Ok(std::move(val));
				else if(val.isLoopBreak())
					break;
				else if(val.isLoopContinue())
					; // do nothing
			}

			if(this->update != nullptr)
				TRY(this->update->evaluate(ev));
		}

		return EvalResult::ofVoid();
	}
}
