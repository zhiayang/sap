// func.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"

#include "interp/tree.h"
#include "interp/state.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<const Type*> FunctionDecl::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		assert(this->type != nullptr);
		return Ok(this->type);
	}

	ErrorOr<const Type*> BuiltinFunctionDefn::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return this->declaration->typecheck(cs);
	}


	ErrorOr<std::optional<Value>> FunctionDecl::evaluate(Interpreter* cs) const
	{
		// TODO
		return ErrFmt("not implemented");
	}

	ErrorOr<std::optional<Value>> BuiltinFunctionDefn::evaluate(Interpreter* cs) const
	{
		// TODO
		return ErrFmt("not implemented");
	}
}
