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
	struct InputArg
	{
		std::optional<const Type*> type;
		std::optional<std::string> name;
	};

	struct ExpectedParam
	{
		std::string name;
		const Type* type;
		const cst::Expr* default_value;
	};

	using ExpectedParams = std::vector<ExpectedParam>;

	struct FinalArg
	{
		const Type* param_type;

		using ArgIdxOrDefault = Either<size_t, const cst::Expr*>;
		Either<ArgIdxOrDefault, std::vector<ArgIdxOrDefault>> value;
	};

	struct ArrangedArguments
	{
		// the vector implies a variadic
		using ArgList = std::vector<FinalArg>;

		ArgList arguments;
		int coercion_cost;
	};

	ErrorOr<ArrangedArguments> arrangeCallArguments( //
	    const Typechecker* ts,                       //
	    const ExpectedParams& expected_params,       //
	    const std::vector<InputArg>& arguments,      //
	    const char* fn_or_struct,                    //
	    const char* thing_name,                      //
	    const char* thing_name2);

}
