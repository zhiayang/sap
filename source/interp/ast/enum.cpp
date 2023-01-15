// enum.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/misc.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<EvalResult> EnumDecl::evaluate(Evaluator* ev) const
	{
		return ErrFmt("not implemented");
	}

	ErrorOr<TCResult> EnumDecl::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		return ErrFmt("not implemented");
	}

	ErrorOr<EvalResult> EnumDefn::evaluate(Evaluator* ev) const
	{
		return ErrFmt("not implemented");
	}

	ErrorOr<TCResult> EnumDefn::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		return ErrFmt("not implemented");
	}
}
