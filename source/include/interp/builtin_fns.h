// builtin_fns.h
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
	struct Evaluator;
	struct Interpreter;

	namespace builtin
	{
		StrErrorOr<EvalResult> bold1(Evaluator* cs, std::vector<Value>& args);
		StrErrorOr<EvalResult> italic1(Evaluator* cs, std::vector<Value>& args);
		StrErrorOr<EvalResult> bold_italic1(Evaluator* cs, std::vector<Value>& args);

		StrErrorOr<EvalResult> apply_style_tio(Evaluator* cs, std::vector<Value>& args);
		StrErrorOr<EvalResult> apply_style_tbo(Evaluator* cs, std::vector<Value>& args);

		StrErrorOr<EvalResult> print(Evaluator* cs, std::vector<Value>& args);
		StrErrorOr<EvalResult> println(Evaluator* cs, std::vector<Value>& args);

		StrErrorOr<EvalResult> load_image(Evaluator* cs, std::vector<Value>& args);

		StrErrorOr<EvalResult> current_style(Evaluator* cs, std::vector<Value>& args);
		StrErrorOr<EvalResult> push_style(Evaluator* cs, std::vector<Value>& args);
		StrErrorOr<EvalResult> pop_style(Evaluator* cs, std::vector<Value>& args);
	}
}
