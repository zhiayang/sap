// for.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::ast
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
			auto cond_res = TRY(this->cond->typecheck(ts, Type::makeBool()));
			if(not cond_res.type()->isBool())
				return ErrMsg(ts, "cannot convert '{}' to boolean condition", cond_res.type());
		}

		if(this->update != nullptr)
			TRY(this->update->typecheck(ts));

		{
			auto t = ts->enterLoopBody();
			TRY(this->body->typecheck(ts));
		}

		return TCResult::ofVoid();
	}

	ErrorOr<TCResult2> ForLoop::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		// everything should be in its own scope.
		auto tree = ts->current()->declareAnonymousNamespace();
		auto _ = ts->pushTree(tree);

		auto ret = std::make_unique<cst::ForLoop>(m_location);

		if(this->init != nullptr)
			ret->init = TRY(this->init->typecheck2(ts)).take_expr();

		if(this->cond != nullptr)
		{
			auto cond_res = TRY(this->cond->typecheck2(ts, Type::makeBool()));
			if(not cond_res.type()->isBool())
				return ErrMsg(ts, "cannot convert '{}' to boolean condition", cond_res.type());

			ret->cond = std::move(cond_res).take_expr();
		}

		if(this->update != nullptr)
			ret->update = TRY(this->update->typecheck2(ts)).take_expr();

		{
			auto t = ts->enterLoopBody();
			ret->body = TRY(this->body->typecheck2(ts)).take<cst::Block>();
		}

		return TCResult2::ofVoid(std::move(ret));
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
				auto cond_val = TRY_VALUE(this->cond->evaluate(ev));
				loop_cond = ev->castValue(std::move(cond_val), Type::makeBool()).getBool();
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
					(void) false; // do nothing
			}

			if(this->update != nullptr)
				TRY(this->update->evaluate(ev));
		}

		return EvalResult::ofVoid();
	}
}
