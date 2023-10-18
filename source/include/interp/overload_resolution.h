// overload_resolution.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <tuple>

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	struct ArrangeArg
	{
		const Type* type;
		std::optional<std::string> name;
	};

	struct ArgPair
	{
		const Type* type;
		const cst::Expr* default_value;
	};

	struct ExpectedParam
	{
		std::string name;
		const Type* type;
		const cst::Expr* default_value;
	};

	using ExpectedParams = std::vector<ExpectedParam>;


	struct ArrangedArguments
	{
		util::hashmap<size_t, std::vector<ArgPair>> param_idx_to_args;
		util::hashmap<size_t, size_t> arg_idx_to_param_idx;
	};

	ErrorOr<ArrangedArguments> arrangeArgumentTypes(const Typechecker* ts,
	    const ExpectedParams& expected,      //
	    const std::vector<ArrangeArg>& args, //
	    const char* fn_or_struct,            //
	    const char* thing_name,              //
	    const char* thing_name2);

	ErrorOr<int> getCallingCost(Typechecker* ts,                         //
	    const ExpectedParams& expected,                                  //
	    const util::hashmap<size_t, std::vector<ArgPair>>& ordered_args, //
	    const char* fn_or_struct,                                        //
	    const char* thing_name,                                          //
	    const char* thing_name2);
}
