// runner.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <memory> // for unique_ptr

#include "defs.h"     // for ErrorOr, ErrFmt
#include "error.h"    // for error
#include "location.h" // for error

#include "interp/tree.h"   // for InlineObject
#include "interp/type.h"   // for Type
#include "interp/state.h"  // for Interpreter
#include "interp/value.h"  // for Value
#include "interp/interp.h" // for Expr, Stmt, FunctionCall, Ident

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
			auto result_or_err = call->evaluate(this);

			if(result_or_err.is_err())
				error(stmt->location, "TODO: handle error");
			else if(not result_or_err->has_value())
				return {};

			auto value = std::move(**result_or_err);
			if(not value.isTreeInlineObj())
				error(stmt->location, "TODO: convert non-inline-obj to inline-obj");

			return std::move(value).takeTreeInlineObj();
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
