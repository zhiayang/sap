// print.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/value.h"
#include "interp/interp.h"

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
		return Ok(EvalResult::of_void());
	}

	ErrorOr<EvalResult> println(Interpreter* cs, std::vector<Value>& args)
	{
		print_values(args);
		zpr::println("");

		return Ok(EvalResult::of_void());
	}
}
