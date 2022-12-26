// builtin.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

#include "util.h"

namespace sap::interp
{
	struct Expr;
	struct Value;
	struct DefnTree;
	struct Interpreter;

	void defineBuiltins(Interpreter* cs, DefnTree* builtin_namespace);
}
