// fstring.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> FStringExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
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

				if(auto t = TRY(call->typecheck(ts)).type(); not t->isString())
					return ErrMsg(loc, "`to_string` method returned non-string type '{}'", t);

				*expr = std::move(call);
			}
		}

		return TCResult::ofRValue(Type::makeString());
	}


	ErrorOr<EvalResult> FStringExpr::evaluate_impl(Evaluator* ev) const
	{
		std::u32string str {};

		for(auto& part : this->parts)
		{
			if(auto expr = std::get_if<std::unique_ptr<Expr>>(&part); expr != nullptr)
				str += TRY_VALUE((*expr)->evaluate(ev)).getUtf32String();
			else
				str += std::get<std::u32string>(part);
		}

		return EvalResult::ofValue(Value::string(std::move(str)));
	}
}
