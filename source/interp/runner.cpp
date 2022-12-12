// runner.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "interp/tree.h"
#include "interp/state.h"

#define dcast(T, E) dynamic_cast<T*>(E)
#define cdcast(T, E) dynamic_cast<const T*>(E)

namespace sap::interp
{
	std::unique_ptr<tree::InlineObject> Interpreter::run(const Stmt* stmt)
	{
		if(auto res = stmt->typecheck(this); res.is_err())
			error(stmt->location, "{}", res.take_error());

		if(auto call = cdcast(FunctionCall, stmt); call != nullptr)
		{
			zpr::println("calling function {}", dcast(Ident, call->callee.get())->name);
			auto result = call->evaluate(this);
		}
		else
		{
			error("interp", "unsupported expression");
		}

		return {};
	}

	ErrorOr<Value> Interpreter::evaluate(const Expr* expr)
	{
		return ErrFmt("aoeu");
	}


	const Type* Expr::type(Interpreter* cs) const
	{
		if(m_type != nullptr)
			return m_type;

		error(this->location, "cannot get type of expression that was not typechecked!");
	}
}
