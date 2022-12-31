// print.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for stringFromU32String

#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp::builtin
{
	static void print_values(std::vector<Value>& values)
	{
		for(size_t i = 0; i < values.size(); i++)
			zpr::print("{}{}", i == 0 ? "" : " ", unicode::stringFromU32String(values[i].toString()));
	}

	ErrorOr<EvalResult> print(Interpreter* cs, std::vector<Value>& args)
	{
		print_values(args);
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> println(Interpreter* cs, std::vector<Value>& args)
	{
		print_values(args);
		zpr::println("");

		return EvalResult::ofVoid();
	}
}
