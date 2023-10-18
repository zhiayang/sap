// overload_resolution.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <tuple>

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/interp.h"
#include "interp/overload_resolution.h"

namespace sap::interp::ast
{
	ErrorOr<ArrangedArguments> arrangeArgumentTypes(const Typechecker* ts,
	    const ExpectedParams& expected,      //
	    const std::vector<ArrangeArg>& args, //
	    const char* fn_or_struct,            //
	    const char* thing_name,              //
	    const char* thing_name2)
	{
		// note: we can't really assume anything about the number of arguments, since we might have less
		// than expected (due to optional args), but also more than expected (due to variadic arrays)
		ArrangedArguments ret {};

		util::hashmap<std::string, size_t> param_names {};
		for(size_t i = 0; i < expected.size(); i++)
			param_names[expected[i].name] = i;

		size_t variadic_param_idx = expected.size();
		for(size_t i = 0; i < expected.size(); i++)
		{
			if(expected[i].type->isVariadicArray())
			{
				variadic_param_idx = i;
				break;
			}
		}

		size_t param_idx = 0;
		size_t have_named = false;
		for(size_t arg_idx = 0; arg_idx < args.size(); arg_idx++)
		{
			auto& arg = args[arg_idx];
			auto& exp_param = expected[param_idx];

			auto get_arg_pair = [&](auto&& argument) -> ArgPair {
				return {
					.type = argument.type,
					.default_value = exp_param.default_value,
				};
			};

			if(arg.name.has_value())
			{
				have_named = true;
				if(auto it = param_names.find(*arg.name); it == param_names.end())
				{
					return ErrMsg(ts, "{} has no {} named '{}'", fn_or_struct, thing_name, *arg.name);
				}
				else
				{
					if(auto tmp = ret.param_idx_to_args.find(it->second); tmp != ret.param_idx_to_args.end())
						return ErrMsg(ts, "{} '{}' already specified", thing_name2, *arg.name);

					ret.param_idx_to_args[it->second].push_back(get_arg_pair(arg));
					ret.arg_idx_to_param_idx[arg_idx] = it->second;
				}
			}
			else
			{
				// variadics are never specified by name. here, check the index of the current argument
				if(param_idx >= variadic_param_idx)
				{
					auto param_type = expected[variadic_param_idx].type;

					if(param_type->isVariadicArray())
					{
						ret.param_idx_to_args[param_idx].push_back(get_arg_pair(arg));
						ret.arg_idx_to_param_idx[arg_idx] = param_idx;

						// note: don't increment param_idx -- lump all the args for
						// this variadic to the same index.
					}
					else
					{
						return ErrMsg(ts, "too many arguments specified; expected at most {}, got {}", expected.size(),
						    param_idx);
					}
				}
				else
				{
					if(have_named)
						return ErrMsg(ts, "positional {} not allowed after named {}s", thing_name, thing_name);

					ret.param_idx_to_args[param_idx].push_back(get_arg_pair(arg));
					ret.arg_idx_to_param_idx[arg_idx] = param_idx;
					param_idx += 1;
				}
			}
		}

		return Ok(std::move(ret));
	}




	ErrorOr<int> getCallingCost(Typechecker* ts,
	    const ExpectedParams& expected,
	    const util::hashmap<size_t, std::vector<ArgPair>>& param_idx_to_args,
	    const char* fn_or_struct,
	    const char* thing_name,
	    const char* thing_name2)
	{
		int cost = 0;

		size_t variadic_param_idx = expected.size();
		const Type* variadic_element_type = nullptr;

		for(size_t i = 0; i < expected.size(); i++)
		{
			if(auto t = expected[i].type; t->isVariadicArray())
			{
				variadic_param_idx = i;
				variadic_element_type = t->arrayElement();
				break;
			}
		}

		int variadic_cost = 0;
		for(size_t param_idx = 0; param_idx < expected.size(); param_idx++)
		{
			auto arg_pairs = TRY(([&](size_t k) -> ErrorOr<std::vector<ArgPair>> {
				if(not param_idx_to_args.contains(k))
				{
					if(expected[k].default_value == nullptr)
					{
						return ErrMsg(ts, "missing {} '{}'", thing_name2, expected[k].name);
					}
					else
					{
						auto asdf = std::vector<ArgPair> { {
							.type = expected[k].default_value->type(),
							.default_value = expected[k].default_value,
						} };

						return Ok(std::move(asdf));
					}
				}
				else
				{
					return Ok(param_idx_to_args.at(k));
				}
			}(param_idx)));

			for(auto& ap : arg_pairs)
			{
				auto arg_type = ap.type;
				auto is_deferred = ap.type == nullptr;

				if(param_idx == variadic_param_idx)
				{
					if(variadic_element_type == nullptr)
						return ErrMsg(ts, "too many arguments (expected {})", expected.size());

					if(arg_type == Type::makeArray(variadic_element_type, /* variadic */ true))
						;
					else if(arg_type == variadic_element_type)
						variadic_cost += 1;
					else if(variadic_element_type->isAny())
						variadic_cost += 5;
					else if(not is_deferred && ts->canImplicitlyConvert(arg_type, variadic_element_type))
						variadic_cost += 2;
					else
						return ErrMsg(ts, "mismatched types in variadic argument; expected '{}', got '{}",
						    variadic_element_type, arg_type ? arg_type : Type::makeVoid());

					continue;
				}

				auto param_type = expected[param_idx].type;

				// no conversion = no cost
				if(arg_type == param_type)
					continue;

				// if we are forced to defer typechecking (ie. we don't have a type for this argument),
				// then skip it -- it's free for now.
				if(is_deferred)
					continue;

				assert(arg_type != nullptr);

				if(param_type->isAny())
				{
					cost += 5;
				}
				else if(param_type->isVariadicArray() && ts->canImplicitlyConvert(arg_type, param_type->arrayElement()))
				{
					cost += 1;
				}
				else if(ts->canImplicitlyConvert(arg_type, param_type))
				{
					cost += 1;
				}
				else
				{
					return ErrMsg(ts, "mismatched types for {} {}: expected '{}', got '{}'", //
					    thing_name, 1 + param_idx, param_type, arg_type);
				}
			}
		}

		return Ok(cost + variadic_cost);
	}

}
