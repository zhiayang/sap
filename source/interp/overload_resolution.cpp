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
	ErrorOr<ArrangedArguments> arrangeCallArguments(const Typechecker* ts,
	    const ExpectedParams& expected,
	    const std::vector<InputArg>& arguments,
	    const char* fn_or_struct,
	    const char* argument_str,
	    const char* parameter_str)
	{
		using ArgIdxOrDefault = FinalArg::ArgIdxOrDefault;
		util::hashmap<zst::str_view, size_t> param_name_to_idx {};

		for(size_t i = 0; i < expected.size(); i++)
			param_name_to_idx[expected[i].name] = i;

		int coercion_cost = 0;
		std::unordered_map<size_t, FinalArg> parameter_values {};

		auto try_calculate_arg_cost =
		    [&](size_t param_idx, const Type* arg_type, const Type* param_type) -> ErrorOr<int> {
			if(arg_type == param_type)
				return Ok(0);
			else if(param_type->isAny())
				return Ok(5);
			else if(ts->canImplicitlyConvert(arg_type, param_type))
				return Ok(1);
			else if(param_type->isVariadicArray() && ts->canImplicitlyConvert(arg_type, param_type->arrayElement()))
				return Ok(1);

			return ErrMsg(ts, "mismatched types for {} {}: expected '{}', got '{}'", //
			    parameter_str, 1 + param_idx, param_type, arg_type);
		};

		/*
		    strategy: loop through all arguments, and match them to parameters.
		*/
		bool saw_named_arg = false;
		bool got_direct_var_array = false;
		size_t positional_param_idx = 0;
		for(size_t arg_idx = 0; arg_idx < arguments.size(); arg_idx++)
		{
			auto& arg = arguments[arg_idx];
			if(arg.name.has_value())
			{
				saw_named_arg = true;
				if(auto it = param_name_to_idx.find(*arg.name); it != param_name_to_idx.end())
				{
					auto param_idx = it->second;
					if(auto it2 = parameter_values.find(param_idx); it2 != parameter_values.end())
					{
						std::string extra {};
						if(auto& val = it2->second.value; val.is_left() && val.left().is_left())
						{
							extra = zpr::sprint(" (previously given by positional {} {})", argument_str,
							    1 + val.left().left());
						}
						return ErrMsg(ts, "duplicate {} '{}'{}", argument_str, *arg.name, extra);
					}

					// deferred args are 'free'
					if(arg.type.has_value())
						coercion_cost += TRY(try_calculate_arg_cost(param_idx, *arg.type, expected[param_idx].type));

					parameter_values.emplace(param_idx,
					    FinalArg {
					        .param_type = expected[param_idx].type,
					        .value = Left<ArgIdxOrDefault>(Left(arg_idx)),
					    });
				}
				else
				{
					return ErrMsg(ts, "{} has no {} named '{}'", fn_or_struct, parameter_str, *arg.name);
				}
			}
			else
			{
				if(positional_param_idx >= expected.size())
				{
					return ErrMsg(ts, "too many {}s provided for {}; expected at most {}", argument_str, fn_or_struct,
					    expected.size());
				}

				auto handle_variadic_arg =
				    [ts, parameter_str, &parameter_values, &expected, &arguments,
				        &try_calculate_arg_cost](size_t param_idx, size_t arg_idx) -> ErrorOr<int> {
					auto& param = expected[param_idx];
					auto& arg = arguments[arg_idx];

					auto [it, _] = parameter_values.try_emplace(param_idx,
					    FinalArg {
					        .param_type = param.type,
					        .value = Right<std::vector<ArgIdxOrDefault>>(),
					    });

					it->second.value.right().push_back(Left(arg_idx));

					if(not arg.type.has_value())
						return Ok(0);

					auto var_elm_type = param.type->arrayElement();

					// note: we want to customise the error message here, so explicitly check for error.
					auto maybe_cost = try_calculate_arg_cost(param_idx, *arg.type, var_elm_type);
					if(maybe_cost.is_err())
					{
						return ErrMsg(ts, "mismatched types for variadic {}; expected '{}', got '{}", parameter_str,
						    var_elm_type, *arg.type);
					}

					return Ok(maybe_cost.unwrap());
				};

				// if we saw a named arg before, then the only acceptable case
				// is that our positional_param_idx is a variadic -- then all the unnamed args
				// just get swallowed by that variadic.
				if(saw_named_arg)
				{
					auto& param = expected[positional_param_idx];
					if(not param.type->isVariadicArray())
						return ErrMsg(ts, "positional {}s are not allowed after named ones", argument_str);

					if(got_direct_var_array)
						return ErrMsg(ts, "cannot provide more {}s after spread array", argument_str);

					coercion_cost += TRY(handle_variadic_arg(positional_param_idx, arg_idx));
				}
				else
				{
					// if the current parameter index corresponds to a variadic param,
					// then we should not advance the positional index; the rest of the args
					// should get consumed by the variadic param.
					auto param_idx = positional_param_idx;
					auto& param = expected[param_idx];

					if(not param.type->isVariadicArray())
					{
						positional_param_idx += 1;

						// deferred args are 'free'
						if(arg.type.has_value())
							coercion_cost += TRY(try_calculate_arg_cost(param_idx, *arg.type, param.type));

						parameter_values.emplace(param_idx,
						    FinalArg {
						        .param_type = expected[param_idx].type,
						        .value = Left<ArgIdxOrDefault>(Left(arg_idx)),
						    });
					}
					else
					{
						coercion_cost += TRY(handle_variadic_arg(positional_param_idx, arg_idx));
					}
				}
			}
		}


		ArrangedArguments::ArgList arranged {};

		// now convert our map into the right format.
		for(size_t i = 0; i < expected.size(); i++)
		{
			auto it = parameter_values.find(i);
			if(it == parameter_values.end())
			{
				// check whether there is a default; if not, fail.
				if(expected[i].type->isVariadicArray())
				{
					// default value is an empty array
					arranged.push_back(FinalArg {
					    .param_type = expected[i].type,
					    .value = Right<std::vector<ArgIdxOrDefault>>(),
					});
				}
				else if(expected[i].default_value != nullptr)
				{
					arranged.push_back(FinalArg {
					    .param_type = expected[i].type,
					    .value = Left<ArgIdxOrDefault>(Right(expected[i].default_value)),
					});
				}
				else
				{
					return ErrMsg(ts, "missing {} for {} '{}'", argument_str, parameter_str, expected[i].name);
				}
			}
			else
			{
				arranged.push_back(std::move(it->second));
			}
		}

		return Ok(ArrangedArguments {
		    .arguments = std::move(arranged),
		    .coercion_cost = coercion_cost,
		});
	}
}
