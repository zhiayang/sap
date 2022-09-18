// builtin.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "defs.h"

#include <memory>

namespace sap::interp
{
	struct Expr;
	struct Interpreter;

	namespace builtin
	{
		std::unique_ptr<Expr> bold1(Interpreter* cs, const std::vector<const Expr*>& args);
	}
}
