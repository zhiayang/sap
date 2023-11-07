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
					if(arg.type.has_value())
						coercion_cost += TRY(try_calculate_arg_cost(*arg.type, param.type));
				}
				else
				{
					/*
					    AAAAAA FIXME AAAAAAA

					    problem:

					    decl: make_text(texts: [Inline...], glue: bool)
					    call: make_text(glue: true, ...)

					    when we encounter `glue: true`, we are trying to solve for param_idx=0, which is the
					    variadic array. we then immediately terminate because we see a named argument, and
					    the pack becomes empty.

					    if we see a named arg while doing variadic, we probably want to just skip it
					    and come back to it later?

					    a side effect of this is that you can do cursed stuff like this now:
					    cursed(x: 1, a, b, y: 2, c, z: 0, d)

					    which will throw [a, b, c, d] into the pack and solve x, y, z correctly. is that a bad
					    thing? idk man. it does seem ultra cursed.

					    ALSO, we need to check for extraneous args at the end of this outer loop if
					    arg_idx < arguments.size() -- these are bogus args that don't correspond to any params.
					*/

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
							/* FIXME: see above */ continue;

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

					arranged.push_back({
					    .param_type = param.type,
					    .value = Right(std::move(pack)),
					});
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

					if(arg.type.has_value())
						coercion_cost += TRY(try_calculate_arg_cost(*arg.type, param.type));

					arranged.push_back({
					    .param_type = param.type,
					    .value = Left<ArgIdxOrDefault>(Left(it->second)),
					});
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
}
