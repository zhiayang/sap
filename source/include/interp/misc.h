// misc.h
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

	template <typename T, bool MoveValue>
	StrErrorOr<std::unordered_map<size_t, T>> arrange_arguments(                                                //
	    const std::vector<std::tuple<std::string, const Type*, const Expr*>>& expected,                      //
	    std::conditional_t<MoveValue, std::vector<ArrangeArg<T>>&&, const std::vector<ArrangeArg<T>>&> args, //
	    const char* fn_or_struct,                                                                            //
	    const char* thing_name,                                                                              //
	    const char* thing_name2)
	{
		// because of optional arguments, we can have fewer arguments than parameters, but not the other way around
		if(args.size() > expected.size())
			return ErrFmt("wrong number of {}: got {}, expected at most {}", thing_name, args.size(), expected.size());

		std::unordered_map<size_t, T> ordered_args {};

		std::unordered_map<std::string, size_t> param_names {};
		for(size_t i = 0; i < expected.size(); i++)
			param_names[std::get<0>(expected[i])] = i;

		size_t cur_idx = 0;
		size_t have_named = false;
		for(auto& arg : args)
		{
			if(arg.name.has_value())
			{
				have_named = true;
				if(auto it = param_names.find(*arg.name); it == param_names.end())
				{
					return ErrFmt("{} has no {} named '{}'", fn_or_struct, thing_name, *arg.name);
				}
				else
				{
					if(auto tmp = ordered_args.find(it->second); tmp != ordered_args.end())
						return ErrFmt("{} '{}' already specified", thing_name2, *arg.name);

					if constexpr(MoveValue)
						ordered_args[it->second] = std::move(arg.value);
					else
						ordered_args[it->second] = arg.value;
				}
			}
			else
			{
				// NOTE: this should have been caught in the parser, but just in case...
				if(have_named)
					return ErrFmt("positional {} not allowed after named {}s", thing_name, thing_name);

				if constexpr(MoveValue)
					ordered_args[cur_idx++] = std::move(arg.value);
				else
					ordered_args[cur_idx++] = arg.value;
			}
		}

		return Ok(std::move(ordered_args));
	}


	inline constexpr auto arrange_argument_types = arrange_arguments<const Type*, /* move: */ false>;
	inline constexpr auto arrange_argument_values = arrange_arguments<Value, /* move: */ true>;


	inline StrErrorOr<int> get_calling_cost(                                               //
	    Typechecker* ts,                                                                //
	    const std::vector<std::tuple<std::string, const Type*, const Expr*>>& expected, //
	    std::unordered_map<size_t, const Type*>& ordered_args,                          //
	    const char* fn_or_struct,                                                       //
	    const char* thing_name,                                                         //
	    const char* thing_name2)
	{
		int cost = 0;
		for(size_t i = 0; i < ordered_args.size(); i++)
		{
			auto arg_type = ordered_args[i];
			if(arg_type == nullptr)
			{
				if(auto default_value = std::get<2>(expected[i]); default_value == nullptr)
					return ErrFmt("missing {} '{}'", thing_name2, std::get<0>(expected[i]));
				else
					arg_type = TRY(default_value->typecheck(ts)).type();
			}

			// if the param is an any, we can just do it, but with extra cost.
			if(auto param_type = std::get<1>(expected[i]); arg_type != param_type)
			{
				if(param_type->isAny())
				{
					cost += 2;
				}
				else if(ts->canImplicitlyConvert(arg_type, param_type))
				{
					cost += 1;
				}
				else
				{
					return ErrFmt("mismatched types for {} {}: got '{}', expected '{}'", //
					    thing_name, 1 + i, arg_type, param_type);
				}
			}
			else
			{
				cost += 0;
			}
		}

		return Ok(cost);
	}

}
