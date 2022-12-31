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
	template <typename T, typename ArgProcessor>
	ErrorOr<std::pair<std::unordered_map<size_t, T>, std::vector<const Type*>>> arrange_arguments( //
	    Interpreter* cs,                                                                           //
	    const std::vector<std::tuple<std::string, const Type*, const Expr*>>& expected,            //
	    const std::vector<FunctionCall::Arg>& args,                                                //
	    const char* fn_or_struct,                                                                  //
	    const char* thing_name,                                                                    //
	    const char* thing_name2,                                                                   //
	    ArgProcessor&& ap)
	{
		// because of optional arguments, we can have fewer arguments than parameters, but not the other way around
		if(args.size() > expected.size())
			return ErrFmt("wrong number of {}: got {}, expected at most {}", thing_name, args.size(), expected.size());

		std::unordered_map<size_t, T> ordered_args {};

		std::vector<const Type*> param_types {};
		std::unordered_map<std::string, size_t> param_names {};
		for(size_t i = 0; i < expected.size(); i++)
		{
			param_names[std::get<0>(expected[i])] = i;
			param_types.push_back(std::get<1>(expected[i]));
		}

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

					ordered_args[it->second] = TRY(ap(arg));
				}
			}
			else
			{
				// NOTE: this should have been caught in the parser, but just in case...
				if(have_named)
					return ErrFmt("positional {} not allowed after named {}s", thing_name, thing_name);

				ordered_args[cur_idx++] = TRY(ap(arg));
			}
		}

		return Ok(std::make_pair(std::move(ordered_args), std::move(param_types)));
	}

	inline ErrorOr<int> get_calling_cost(                                                       //
	    Interpreter* cs,                                                                        //
	    const std::vector<std::tuple<std::string, const Type*, const Expr*>>& expected,         //
	    std::pair<std::unordered_map<size_t, const Type*>, std::vector<const Type*>>& arranged, //
	    const char* fn_or_struct,                                                               //
	    const char* thing_name,                                                                 //
	    const char* thing_name2)
	{
		auto& [ordered_args, ordered_types] = arranged;

		int cost = 0;
		for(size_t i = 0; i < ordered_args.size(); i++)
		{
			auto arg_type = ordered_args[i];
			if(arg_type == nullptr)
			{
				if(auto default_value = std::get<2>(expected[i]); default_value == nullptr)
					return ErrFmt("missing {} '{}'", thing_name2, std::get<0>(expected[i]));
				else
					arg_type = TRY(default_value->typecheck(cs));
			}

			// if the param is an any, we can just do it, but with extra cost.
			if(auto param_type = ordered_types[i]; arg_type != param_type)
			{
				if(param_type->isAny())
				{
					cost += 2;
				}
				else
				{
					return ErrFmt("mismatched types for {} {}: got '{}', expected '{}'", //
					    thing_name, 1 + i, arg_type, param_type);
				}
			}
			else
			{
				cost += 1;
			}
		}

		return Ok(cost);
	}

}
