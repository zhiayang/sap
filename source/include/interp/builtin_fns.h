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
		ErrorOr<EvalResult> bold1(Evaluator* cs, std::vector<Value>& args);
		ErrorOr<EvalResult> italic1(Evaluator* cs, std::vector<Value>& args);
		ErrorOr<EvalResult> bold_italic1(Evaluator* cs, std::vector<Value>& args);

		ErrorOr<EvalResult> apply_style_tio(Evaluator* cs, std::vector<Value>& args);
		ErrorOr<EvalResult> apply_style_tbo(Evaluator* cs, std::vector<Value>& args);

		ErrorOr<EvalResult> print(Evaluator* cs, std::vector<Value>& args);
		ErrorOr<EvalResult> println(Evaluator* cs, std::vector<Value>& args);

		ErrorOr<EvalResult> load_image(Evaluator* cs, std::vector<Value>& args);
		ErrorOr<EvalResult> include_file(Evaluator* cs, std::vector<Value>& args);

		ErrorOr<EvalResult> current_style(Evaluator* cs, std::vector<Value>& args);
		ErrorOr<EvalResult> push_style(Evaluator* cs, std::vector<Value>& args);
		ErrorOr<EvalResult> pop_style(Evaluator* cs, std::vector<Value>& args);

		ErrorOr<EvalResult> to_string(Evaluator* cs, std::vector<Value>& args);
	}
}
