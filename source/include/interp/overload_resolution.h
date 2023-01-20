// overload_resolution.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <tuple>

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/interp.h"

namespace sap::interp
{
	template <typename T>
	struct ArrangeArg
	{
		std::optional<std::string> name;
		T value;
	};

	template <typename TsEv, typename T, bool MoveValue>
	extern ErrorOr<std::unordered_map<size_t, T>> arrange_arguments(const TsEv* ts_ev,
	    const std::vector<std::tuple<std::string, const Type*, const Expr*>>& expected,                      //
	    std::conditional_t<MoveValue, std::vector<ArrangeArg<T>>&&, const std::vector<ArrangeArg<T>>&> args, //
	    const char* fn_or_struct,                                                                            //
	    const char* thing_name,                                                                              //
	    const char* thing_name2);

	inline constexpr auto arrangeArgumentTypes = arrange_arguments<Typechecker, const Type*, /* move: */ false>;
	inline constexpr auto arrangeArgumentValues = arrange_arguments<Evaluator, Value, /* move: */ true>;

	ErrorOr<int> getCallingCost(Typechecker* ts,                                        //
	    const std::vector<std::tuple<std::string, const Type*, const Expr*>>& expected, //
	    std::unordered_map<size_t, const Type*>& ordered_args,                          //
	    const char* fn_or_struct,                                                       //
	    const char* thing_name,                                                         //
	    const char* thing_name2);
}
