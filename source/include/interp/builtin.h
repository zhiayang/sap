// builtin.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "defs.h"

#include <memory>

namespace sap::interp
{
	struct Expr;
	struct Value;
	struct Interpreter;

	namespace builtin
	{
		ErrorOr<std::optional<Value>> bold1(Interpreter* cs, std::vector<Value>& args);
	}
}
