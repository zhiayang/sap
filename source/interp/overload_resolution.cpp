// overload_resolution.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <tuple>

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/interp.h"
#include "interp/overload_resolution.h"

namespace sap::interp
{
	using ExpectedParams = std::vector<std::tuple<std::string, const Type*, const Expr*>>;

	template <typename T, bool Move>
	using ArgList = std::conditional_t<Move, std::vector<ArrangeArg<T>>&&, const std::vector<ArrangeArg<T>>&>;

	template <typename TsEv, typename T, bool MoveValue>
	ErrorOr<ArrangedArguments<T>> arrange_arguments(const TsEv* ts_ev,
		const ExpectedParams& expected, //
		ArgList<T, MoveValue> args,     //
		const char* fn_or_struct,       //
		const char* thing_name,         //
		const char* thing_name2)
	{
		// note: we can't really assume anything about the number of arguments, since we might have less
		// than expected (due to optional args), but also more than expected (due to variadic arrays)
		ArrangedArguments<T> ret {};

		std::unordered_map<std::string, size_t> param_names {};
		for(size_t i = 0; i < expected.size(); i++)
			param_names[std::get<0>(expected[i])] = i;

		size_t cur_idx = 0;
		size_t have_named = false;
		for(size_t arg_idx = 0; arg_idx < args.size(); arg_idx++)
		{
			auto& arg = args[arg_idx];

			auto get_arg_pair = [](auto&& arg) -> ArgPair<T> {
				if constexpr(MoveValue)
					return { std::move(arg.value), arg.deferred_typecheck };
				else
					return { arg.value, arg.deferred_typecheck };
			};

			if(arg.name.has_value())
			{
				have_named = true;
				if(auto it = param_names.find(*arg.name); it == param_names.end())
				{
					return ErrMsg(ts_ev, "{} has no {} named '{}'", fn_or_struct, thing_name, *arg.name);
				}
				else
				{
					if(auto tmp = ret.param_idx_to_arg.find(it->second); tmp != ret.param_idx_to_arg.end())
						return ErrMsg(ts_ev, "{} '{}' already specified", thing_name2, *arg.name);

					ret.param_idx_to_arg[it->second] = get_arg_pair(arg);
					ret.arg_idx_to_param_idx[arg_idx] = it->second;
				}
			}
			else
			{
				if(have_named)
					return ErrMsg(ts_ev, "positional {} not allowed after named {}s", thing_name, thing_name);

				// variadics are never specified by name. here, check the index of the current argument
				if(cur_idx + 1 > expected.size())
				{
					auto param_type = std::get<1>(expected.back());

					if(param_type->isVariadicArray())
					{
						ret.param_idx_to_arg[cur_idx] = get_arg_pair(arg);
						ret.arg_idx_to_param_idx[arg_idx] = cur_idx;
						cur_idx += 1;
					}
					else
					{
						return ErrMsg(ts_ev, "too many arguments specified; expected at most {}, got {}",
							expected.size(), cur_idx);
					}
				}
				else
				{
					ret.param_idx_to_arg[cur_idx] = get_arg_pair(arg);
					ret.arg_idx_to_param_idx[arg_idx] = cur_idx;
					cur_idx += 1;
				}
			}
		}

		return Ok(std::move(ret));
	}

	template ErrorOr<ArrangedArguments<const Type*>>    //
	arrange_arguments<Typechecker, const Type*, false>( //
		const Typechecker*,
		const ExpectedParams&,
		const std::vector<ArrangeArg<const Type*>>&,
		const char*,
		const char*,
		const char*);

	template ErrorOr<ArrangedArguments<Value>> //
	arrange_arguments<Evaluator, Value, true>( //
		const Evaluator*,
		const ExpectedParams&,
		std::vector<ArrangeArg<Value>>&&,
		const char*,
		const char*,
		const char*);





	ErrorOr<int> getCallingCost(Typechecker* ts,
		const std::vector<std::tuple<std::string, const Type*, const Expr*>>& expected,
		const std::unordered_map<size_t, ArgPair<const Type*>>& ordered_args,
		const char* fn_or_struct,
		const char* thing_name,
		const char* thing_name2)
	{
		int cost = 0;

		const Type* variadic_element_type = nullptr;
		if(expected.size() > 0)
		{
			if(auto t = std::get<1>(expected.back()); t->isVariadicArray())
				variadic_element_type = t->arrayElement();
		}

		int variadic_cost = 0;
		for(size_t i = 0; i < ordered_args.size(); i++)
		{
			const Type* arg_type = nullptr;
			bool is_deferred = false;
			if(not ordered_args.contains(i))
			{
				if(auto default_value = std::get<2>(expected[i]); default_value == nullptr)
					return ErrMsg(ts, "missing {} '{}'", thing_name2, std::get<0>(expected[i]));
				else
					arg_type = TRY(default_value->typecheck(ts)).type();
			}
			else
			{
				arg_type = ordered_args.at(i).value;
				is_deferred = ordered_args.at(i).deferred_typecheck;
			}

			if(i >= expected.size())
			{
				if(variadic_element_type == nullptr)
					return ErrMsg(ts, "too many arguments (expected {})", expected.size());

				if(arg_type == variadic_element_type)
					variadic_cost += 1;
				else if(variadic_element_type->isAny())
					variadic_cost += 3;
				else if(ts->canImplicitlyConvert(arg_type, variadic_element_type))
					variadic_cost += 2;

				continue;
			}

			// if the param is an any, we can just do it, but with extra cost.
			auto param_type = std::get<1>(expected[i]);

			// no conversion = no cost
			if(arg_type == param_type)
				continue;

			// if we are forced to defer typechecking (ie. we don't have a type for this argument),
			// then skip it -- it's free for now.
			if(is_deferred)
				continue;

			if(param_type->isAny())
			{
				cost += 2;
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
				return ErrMsg(ts, "mismatched types for {} {}: got '{}', expected '{}'", //
					thing_name, 1 + i, arg_type, param_type);
			}
		}

		return Ok(cost + variadic_cost);
	}

}
