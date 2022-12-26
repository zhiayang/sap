// print.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/value.h"
#include "interp/state.h"

namespace sap::interp::builtin
{
	ErrorOr<std::optional<Value>> print(Interpreter* cs, std::vector<Value>& args)
	{
		for(size_t i = 0; i < args.size(); i++)
			zpr::print("{}{}", i == 0 ? "" : " ", unicode::stringFromU32String(args[i].toString()));

		zpr::println("");
		return Ok(std::nullopt);
	}
}
