// destructors.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"
#include "interp/ast.h" // for Stmt

namespace sap::interp
{
	ast::Stmt::~Stmt()
	{
	}

	cst::Stmt::~Stmt()
	{
	}
}
