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
	template <typename T>
	struct ArrangeArg
	{
		T value;
		std::optional<std::string> name;
	};

	template <typename T>
	struct ArgPair
	{
		T value;
		bool was_named;
	};

	using ExpectedParams = std::vector<std::tuple<std::string, const Type*, const cst::Expr*>>;

	template <typename T>
	struct ArrangedArguments
	{
		util::hashmap<size_t, std::vector<ArgPair<T>>> param_idx_to_args;
		util::hashmap<size_t, size_t> arg_idx_to_param_idx;
	};

	template <typename T, bool Move>
	ErrorOr<ArrangedArguments<T>> arrange_arguments(const Typechecker* ts,
	    const ExpectedParams& expected,                                                                 //
	    std::conditional_t<Move, std::vector<ArrangeArg<T>>&&, const std::vector<ArrangeArg<T>>&> args, //
	    const char* fn_or_struct,                                                                       //
	    const char* thing_name,                                                                         //
	    const char* thing_name2);


	inline constexpr auto arrangeArgumentTypes = arrange_arguments<const Type*, /* move: */ false>;
	inline constexpr auto arrangeArgumentExprs = arrange_arguments<std::unique_ptr<cst::Expr>, /* move: */ true>;


	ErrorOr<int> getCallingCost(Typechecker* ts,                                             //
	    const std::vector<std::tuple<std::string, const Type*, const cst::Expr*>>& expected, //
	    const util::hashmap<size_t, std::vector<ArgPair<const Type*>>>& ordered_args,        //
	    const char* fn_or_struct,                                                            //
	    const char* thing_name,                                                              //
	    const char* thing_name2);
}
