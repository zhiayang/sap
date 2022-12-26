// func.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/type.h"   // for Type
#include "interp/state.h"  // for Interpreter
#include "interp/value.h"  // for Value
#include "interp/interp.h" // for FunctionDecl, BuiltinFunctionDefn, Declar...

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
