// polymorph.h
// Copyright (c) 2023, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/interp.h"

namespace sap::interp::polymorph
{
	ErrorOr<TCResult> createGenericOverloadSet(Typechecker* ts,
	    const Type* infer,
	    const QualifiedId& name,
	    std::vector<const cst::Declaration*> declarations,
	    const std::vector<ast::FunctionCall::Arg>& explicit_args);
}
