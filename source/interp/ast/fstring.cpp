// fstring.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> FStringExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		std::vector<cst::FStringExpr::Part> new_parts {};

		for(auto& part : this->parts)
		{
			if(auto expr = std::get_if<std::unique_ptr<Expr>>(&part); expr != nullptr)
			{
				auto loc = expr->get()->loc();
				auto call = std::make_unique<FunctionCall>(loc);

				auto name = std::make_unique<Ident>(loc);
				name->name.name = "to_string";

				call->callee = std::move(name);
				call->rewritten_ufcs = true;
				call->arguments.push_back(FunctionCall::Arg {
				    .name = std::nullopt,
				    .value = std::move(*const_cast<std::unique_ptr<Expr>*>(expr)),
				});

				auto cst_call = TRY(call->typecheck(ts)).take_expr();
				if(auto t = cst_call->type(); not t->isString())
					return ErrMsg(loc, "`to_string` method returned non-string type '{}'", t);

				new_parts.push_back(Right(std::move(cst_call)));
			}
			else
			{
				new_parts.push_back(Left(std::get<std::u32string>(part)));
			}
		}

		return TCResult::ofRValue<cst::FStringExpr>(m_location, std::move(new_parts));
	}
}
