// block.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp
{
	ErrorOr<TCResult> Block::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		auto tree = cs->current()->declareAnonymousNamespace();
		auto _ = cs->pushTree(tree);

		for(auto& stmt : this->body)
			TRY(stmt->typecheck(cs));

		return TCResult::ofVoid();
	}

	ErrorOr<EvalResult> Block::evaluate(Interpreter* cs) const
	{
		auto _ = cs->pushFrame();

		for(auto& stmt : this->body)
		{
			auto result = TRY(stmt->evaluate(cs));
			if(not result.isNormal())
				return Ok(std::move(result));
		}

		return EvalResult::ofVoid();
	}
}
