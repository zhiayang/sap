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

	namespace builtin
	{
		ErrorOr<std::optional<Value>> bold1(Interpreter* cs, std::vector<Value>& args);
		ErrorOr<std::optional<Value>> italic1(Interpreter* cs, std::vector<Value>& args);
		ErrorOr<std::optional<Value>> bold_italic1(Interpreter* cs, std::vector<Value>& args);

		ErrorOr<std::optional<Value>> print(Interpreter* cs, std::vector<Value>& args);
		ErrorOr<std::optional<Value>> println(Interpreter* cs, std::vector<Value>& args);
	}
}
