// overload_resolution.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

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

	using ExpectedParams = std::vector<std::tuple<std::string, const Type*, const Expr*>>;

	template <typename TsEv, typename T, bool MoveValue>
	ErrorOr<std::unordered_map<size_t, T>> arrange_arguments(const TsEv* ts_ev,
	    const ExpectedParams& expected,                                                                      //
	    std::conditional_t<MoveValue, std::vector<ArrangeArg<T>>&&, const std::vector<ArrangeArg<T>>&> args, //
	    const char* fn_or_struct,                                                                            //
	    const char* thing_name,                                                                              //
	    const char* thing_name2)
	{
		// note: we can't really assume anything about the number of arguments, since we might have less
		// than expected (due to optional args), but also more than expected (due to variadic arrays)
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
					return ErrMsg(ts_ev, "{} has no {} named '{}'", fn_or_struct, thing_name, *arg.name);
				}
				else
				{
					if(auto tmp = ordered_args.find(it->second); tmp != ordered_args.end())
						return ErrMsg(ts_ev, "{} '{}' already specified", thing_name2, *arg.name);

					if constexpr(MoveValue)
						ordered_args[it->second] = std::move(arg.value);
					else
						ordered_args[it->second] = arg.value;
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
						if constexpr(MoveValue)
							ordered_args[cur_idx++] = std::move(arg.value);
						else
							ordered_args[cur_idx++] = arg.value;
					}
					else
					{
						return ErrMsg(ts_ev, "too many arguments specified; expected at most {}, got {}", expected.size(),
						    cur_idx);
					}
				}
				else
				{
					if constexpr(MoveValue)
						ordered_args[cur_idx++] = std::move(arg.value);
					else
						ordered_args[cur_idx++] = arg.value;
				}
			}
		}

		return Ok(std::move(ordered_args));
	}

	template ErrorOr<std::unordered_map<size_t, const Type*>> //
	arrange_arguments<Typechecker, const Type*, false>(       //
	    const Typechecker*,
	    const ExpectedParams&,
	    const std::vector<ArrangeArg<const Type*>>&,
	    const char*,
	    const char*,
	    const char*);

	template ErrorOr<std::unordered_map<size_t, Value>> //
	arrange_arguments<Evaluator, Value, true>(          //
	    const Evaluator*,
	    const ExpectedParams&,
	    std::vector<ArrangeArg<Value>>&&,
	    const char*,
	    const char*,
	    const char*);

	ErrorOr<int> getCallingCost(Typechecker* ts,
	    const std::vector<std::tuple<std::string, const Type*, const Expr*>>& expected,
	    std::unordered_map<size_t, const Type*>& ordered_args,
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
			auto arg_type = ordered_args[i];
			if(arg_type == nullptr)
			{
				if(auto default_value = std::get<2>(expected[i]); default_value == nullptr)
					return ErrMsg(ts, "missing {} '{}'", thing_name2, std::get<0>(expected[i]));
				else
					arg_type = TRY(default_value->typecheck(ts)).type();
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
