// for.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult2> ForLoop::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		// everything should be in its own scope.
		auto tree = ts->current()->declareAnonymousNamespace();
		auto _ = ts->pushTree(tree);

		auto ret = std::make_unique<cst::ForLoop>(m_location);

		if(this->init != nullptr)
			ret->init = TRY(this->init->typecheck2(ts)).take_stmt();

		if(this->cond != nullptr)
		{
			auto cond_res = TRY(this->cond->typecheck2(ts, Type::makeBool()));
			if(not cond_res.type()->isBool())
				return ErrMsg(ts, "cannot convert '{}' to boolean condition", cond_res.type());

			ret->cond = std::move(cond_res).take_expr();
		}

		if(this->update != nullptr)
			ret->update = TRY(this->update->typecheck2(ts)).take_stmt();

		{
			auto t = ts->enterLoopBody();
			ret->body = TRY(this->body->typecheck2(ts)).take<cst::Block>();
		}

		return TCResult2::ofVoid(std::move(ret));
	}
}
