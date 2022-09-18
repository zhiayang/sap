// runner.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "interp/tree.h"

#define dcast(T, E) dynamic_cast<T*>(E)
#define cdcast(T, E) dynamic_cast<const T*>(E)

namespace sap::interp
{
	std::optional<std::unique_ptr<tree::InlineObject>> runScriptExpression(Interpreter* cs, const Expr* expr)
	{
		if(auto call = cdcast(FunctionCall, expr); call != nullptr)
		{
			zpr::println("calling {}", dcast(Ident, call->callee.get())->name);
			if(not call->callee->type->isFunction())
				error(expr->location, "callee of function call must be a function type, got '{}'", call->callee->type);
		}
		else
		{
			error("interp", "unsupported expression");
		}

		return {};
	}
}
