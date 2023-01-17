// block.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp
{
	ErrorOr<TCResult> Block::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		DefnTree* tree = nullptr;

		if(this->target_scope.has_value())
		{
			tree = ts->current()->lookupOrDeclareScope(this->target_scope->parents, this->target_scope->top_level);
		}
		else
		{
			tree = ts->current()->declareAnonymousNamespace();
		}

		auto _ = ts->pushTree(tree);
		for(auto& stmt : this->body)
			TRY(stmt->typecheck(ts));

		return TCResult::ofVoid();
	}

	ErrorOr<EvalResult> Block::evaluate_impl(Evaluator* ev) const
	{
		auto _ = ev->pushFrame();

		for(auto& stmt : this->body)
		{
			auto result = TRY(stmt->evaluate(ev));
			ev->frame().dropTemporaries();

			if(not result.isNormal())
				return Ok(std::move(result));
		}

		return EvalResult::ofVoid();
	}

	bool Block::checkAllPathsReturn(const Type* return_type)
	{
		for(auto& stmt : this->body)
		{
			// if this returns, we're good already (since we're looking from the back)
			if(auto ret = dynamic_cast<ReturnStmt*>(stmt.get()); ret != nullptr)
			{
				return true;
			}
			else if(auto if_stmt = dynamic_cast<IfStmt*>(stmt.get()); if_stmt != nullptr && if_stmt->else_body)
			{
				if(if_stmt->if_body->checkAllPathsReturn(return_type) && if_stmt->else_body->checkAllPathsReturn(return_type))
					return true;
			}
		}

		return false;
	}
}
