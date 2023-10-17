// block.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult2> Block::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		DefnTree* tree = nullptr;

		if(this->target_scope.has_value())
		{
			// zpr::println("scope = '{}'", *this->target_scope);
			tree = ts->current()->lookupOrDeclareScope(this->target_scope->parents, this->target_scope->top_level);
		}
		else
		{
			tree = ts->current()->declareAnonymousNamespace();
		}

		std::vector<std::unique_ptr<cst::Stmt>> stmts {};

		auto _ = ts->pushTree(tree);
		for(auto& stmt : this->body)
			stmts.push_back(TRY(stmt->typecheck2(ts)).take_stmt());

		return TCResult2::ofVoid<cst::Block>(m_location, std::move(stmts));
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
				if(if_stmt->if_body->checkAllPathsReturn(return_type)
				    && if_stmt->else_body->checkAllPathsReturn(return_type))
				{
					return true;
				}
			}
		}

		return false;
	}
}
