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
	    const char* thing_name,
	    const char* thing_name2)
	{
		util::hashmap<zst::str_view, size_t> arg_name_to_idx {};
		util::hashmap<zst::str_view, size_t> param_name_to_idx {};

		// size_t variadic_param_idx = expected.size();

		for(size_t i = 0; i < expected.size(); i++)
			param_name_to_idx[expected[i].name] = i;

		for(size_t i = 0; i < arguments.size(); i++)
		{
			if(arguments[i].name.has_value())
			{
				arg_name_to_idx[*arguments[i].name] = i;
				if(not param_name_to_idx.contains(*arguments[i].name))
					return ErrMsg(ts, "{} has no {} named '{}'", fn_or_struct, thing_name, *arguments[i].name);
			}
		}

		int coercion_cost = 0;
		ArrangedArguments::ArgList arranged {};

		size_t arg_idx = 0;
		bool saw_named_arg = false;
		for(size_t param_idx = 0; param_idx < expected.size(); param_idx++)
		{
			using ArgIdxOrDefault = FinalArg::ArgIdxOrDefault;

			auto& param = expected[param_idx];

			auto push_default_value_or_error = [ts, thing_name2, &arranged](const auto& param) -> ErrorOr<void> {
				if(param.default_value == nullptr)
					return ErrMsg(ts, "missing {} '{}'", thing_name2, param.name);

				arranged.push_back({
				    .param_type = param.type,
				    .value = Left<ArgIdxOrDefault>(Right(param.default_value)),
				});
				return Ok();
			};

			auto try_calculate_arg_cost = [&](const Type* arg_type, const Type* param_type) -> ErrorOr<int> {
				if(arg_type == param_type)
					return Ok(0);
				else if(param_type->isAny())
					return Ok(5);
				else if(ts->canImplicitlyConvert(arg_type, param_type))
					return Ok(1);
				else if(param_type->isVariadicArray() && ts->canImplicitlyConvert(arg_type, param_type->arrayElement()))
					return Ok(1);

				return ErrMsg(ts, "mismatched types for {} {}: expected '{}', got '{}'", //
				    thing_name, 1 + param_idx, param_type, arg_type);
			};

			// if we're out of args, then it's just the default value.
			// if there's no default value, then it's an error.
			if(arg_idx >= arguments.size())
			{
				TRY(push_default_value_or_error(param));
				continue;
			}

			// ok, we have at least one arg.
			if(param.type->isVariadicArray() || (not saw_named_arg && not arguments[arg_idx].name.has_value()))
			{
				// if this is the last param and it's variadic, then collect the rest of the args.
				// else proceed normally.
				if(not param.type->isVariadicArray())
				{
					auto& arg = arguments[arg_idx];
					arranged.push_back({
					    .param_type = param.type,
					    .value = Left<ArgIdxOrDefault>(Left(arg_idx)),
					});

					// doing a positional argument, so we advance the arg idx.
					arg_idx += 1;

					// if we are forced to defer typechecking (ie. we don't have a type for this argument),
					// then skip it -- it's free for now.
					if(not arg.type.has_value())
						continue;

					coercion_cost += TRY(try_calculate_arg_cost(*arg.type, param.type));
				}
				else
				{
					assert(param.type->isVariadicArray());

					auto var_elm_type = param.type->arrayElement();
					bool got_direct_var_array = false;

					std::vector<ArgIdxOrDefault> pack {};

					for(; arg_idx < arguments.size(); arg_idx++)
					{
						if(got_direct_var_array)
							return ErrMsg(ts, "cannot pass additional arguments after spread array");

						auto& arg = arguments[arg_idx];

						// if the argument is named, it no longer belongs to the variadic param, so bail.
						if(arg.name.has_value())
							break;

						pack.push_back(Left(arg_idx));

						// skip deferred ones
						if(not arg.type.has_value())
							continue;

						if(*arg.type == Type::makeArray(var_elm_type, /* variadic: */ true))
							coercion_cost += 0, got_direct_var_array = true;
						else if(*arg.type == var_elm_type)
							coercion_cost += 1;
						else if(var_elm_type->isAny())
							coercion_cost += 5;
						else if(ts->canImplicitlyConvert(*arg.type, var_elm_type))
							coercion_cost += 2;
						else
							return ErrMsg(ts, "mismatched types in variadic argument; expected '{}', got '{}",
							    var_elm_type, *arg.type);
					}

					arranged.push_back({ .param_type = param.type, .value = Right(std::move(pack)) });
				}
			}
			else
			{
				// all further arguments must be named.
				if(saw_named_arg && not arguments[arg_idx].name.has_value())
					return ErrMsg(ts, "positional {} not allowed after named {}s", thing_name, thing_name);

				saw_named_arg = true;
				if(auto it = arg_name_to_idx.find(param.name); it != arg_name_to_idx.end())
				{
					auto& arg = arguments[it->second];

					// if we are forced to defer typechecking (ie. we don't have a type for this argument),
					// then skip it -- it's free for now.
					if(not arg.type.has_value())
						continue;

					arranged.push_back({
					    .param_type = param.type,
					    .value = Left<ArgIdxOrDefault>(Left(it->second)),
					});
					coercion_cost += TRY(try_calculate_arg_cost(*arg.type, param.type));
				}
				else
				{
					TRY(push_default_value_or_error(param));
				}
			}
		}

		return Ok<ArrangedArguments>({
		    .arguments = std::move(arranged),
		    .coercion_cost = coercion_cost,
		});
	}





#if 0
	ErrorOr<ArrangedArguments> arrangeArgumentTypes(const Typechecker* ts,
	    const ExpectedParams& expected,      //
	    const std::vector<InputArg>& args, //
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
#endif
}
