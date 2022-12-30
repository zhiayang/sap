// struct.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"

namespace sap::interp
{
	ErrorOr<const Type*> StructDecl::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return ErrFmt("A");
	}

	ErrorOr<const Type*> StructDefn::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return ErrFmt("A");
	}

	ErrorOr<EvalResult> StructDecl::evaluate(Interpreter* cs) const
	{
		return ErrFmt("A");
	}

	ErrorOr<EvalResult> StructDefn::evaluate(Interpreter* cs) const
	{
		return ErrFmt("A");
	}
}
