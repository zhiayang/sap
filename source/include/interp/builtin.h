// builtin.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

#include "util.h"

#include "interp/value.h"       // for Interpreter, Value
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp
{
	struct Expr;
	struct Value;
	struct DefnTree;
	struct Interpreter;

	namespace builtin
	{
		ErrorOr<EvalResult> bold1(Interpreter* cs, std::vector<Value>& args);
		ErrorOr<EvalResult> italic1(Interpreter* cs, std::vector<Value>& args);
		ErrorOr<EvalResult> bold_italic1(Interpreter* cs, std::vector<Value>& args);

		ErrorOr<EvalResult> print(Interpreter* cs, std::vector<Value>& args);
		ErrorOr<EvalResult> println(Interpreter* cs, std::vector<Value>& args);
	}
}
