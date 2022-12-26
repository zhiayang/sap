// variable.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/value.h"
#include "interp/state.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<std::optional<Value>> VariableDecl::evaluate(Interpreter* cs) const
	{
		return ErrFmt("A");
	}

	ErrorOr<const Type*> VariableDecl::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		auto ns = cs->current();
		TRY(ns->declare(this));

		return ErrFmt("A");
	}

	ErrorOr<std::optional<Value>> VariableDefn::evaluate(Interpreter* cs) const
	{
		return ErrFmt("A");
	}

	ErrorOr<const Type*> VariableDefn::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return ErrFmt("A");
	}
}
