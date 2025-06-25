// destructors.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"
#include "interp/ast.h"

namespace sap::interp
{
	ast::Stmt::~Stmt()
	{
	}

	cst::Stmt::~Stmt()
	{
	}
}
