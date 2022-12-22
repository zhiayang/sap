// call.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <climits>

#include "sap.h"

#include "interp/tree.h"
#include "interp/state.h"
#include "interp/interp.h"

namespace sap::interp
{
	template <typename T, typename ArgProcessor>
	ErrorOr<std::pair<std::unordered_map<size_t, T>, std::vector<const Type*>>> resolve_argument_order(Interpreter* cs,
		Declaration* decl, const std::vector<FunctionCall::Arg>& args, ArgProcessor&& ap)
	{
		// TODO: check for variables.
		if(auto fdecl = dynamic_cast<FunctionDecl*>(decl); fdecl != nullptr)
		{
			auto& decl_params = fdecl->params();

			// because of optional arguments, we can have fewer arguments than parameters, but not the other way around
			if(args.size() > decl_params.size())
				return ErrFmt("wrong number of arguments: got {}, expected at most {}", args.size(), decl_params.size());

			std::unordered_map<size_t, T> ordered_args {};

			std::vector<const Type*> decl_param_types {};
			std::unordered_map<std::string, size_t> param_names {};
			for(size_t i = 0; i < decl_params.size(); i++)
			{
				param_names[decl_params[i].name] = i;
				decl_param_types.push_back(decl_params[i].type);
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
						return ErrFmt("function has no parameter named '{}'", *arg.name);
					}
					else
					{
						assert(it->second < ordered_args.size());
						if(auto tmp = ordered_args.find(it->second); tmp != ordered_args.end())
							return ErrFmt("argument for parameter '{}' already specified", *arg.name);

						ordered_args[it->second] = TRY(ap(arg));
					}
				}
				else
				{
					// NOTE: this should have been caught in the parser, but just in case...
					if(have_named)
						return ErrFmt("positional arguments not allowed after named arguments");

					ordered_args[cur_idx++] = TRY(ap(arg));
				}
			}

			return Ok(std::make_pair(std::move(ordered_args), std::move(decl_param_types)));
		}
		else
		{
			return ErrFmt("?!");
		}
	}

	static ErrorOr<int> get_calling_cost(Interpreter* cs, Declaration* decl, const std::vector<FunctionCall::Arg>& arguments)
	{
		auto [ordered_args, decl_params_types] = TRY(resolve_argument_order<const Type*>(cs, decl, arguments, [cs](auto& arg) {
			return arg.value->typecheck(cs);
		}));

		int cost = 0;

		for(size_t i = 0; i < ordered_args.size(); i++)
		{
			auto arg_type = ordered_args[i];
			if(arg_type == nullptr)
			{
				if(auto fdecl = dynamic_cast<FunctionDecl*>(decl); fdecl != nullptr)
				{
					auto& params = fdecl->params();
					if(params[i].default_value == nullptr)
						return ErrFmt("missing argument for parameter '{}'", params[i].name);

					arg_type = TRY(params[i].default_value->typecheck(cs));
				}
				else
				{
					return ErrFmt("????");
				}
			}

			if(auto param_type = decl_params_types[i]; arg_type != param_type)
			{
				return ErrFmt("mismatched types for argument {}: got '{}', expected '{}'",  //
					1 + i, arg_type, param_type);
			}

			cost += 1;
		}

		return Ok(cost);
	}


	static ErrorOr<Declaration*> resolve_overload_set(Interpreter* cs, const std::vector<Declaration*>& decls,
		const std::vector<FunctionCall::Arg>& arguments)
	{
		std::vector<Declaration*> best_decls {};
		int best_cost = INT_MAX;

		for(auto decl : decls)
		{
			auto cost = get_calling_cost(cs, decl, arguments);

			if(cost.is_err())
				continue;

			if(*cost == best_cost)
			{
				best_decls.push_back(decl);
			}
			else if(*cost < best_cost)
			{
				best_cost = *cost;
				best_decls = { decl };
			}
		}

		if(best_decls.size() == 1)
			return Ok(best_decls[0]);

		if(best_decls.empty())
			return ErrFmt("no matching function for call");

		return ErrFmt("ambiguous call");
	}



	ErrorOr<const Type*> FunctionCall::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		const Type* fn_type = nullptr;

		// if the lhs is an identifier, we resolve it manually to handle overloading.
		if(auto ident = dynamic_cast<const Ident*>(this->callee.get()); ident != nullptr)
		{
			auto decls = TRY(cs->current()->lookup(ident->name));
			assert(decls.size() > 0);

			Declaration* best_decl = nullptr;
			if(decls.size() == 1)
				best_decl = decls[0];
			else
				best_decl = TRY(resolve_overload_set(cs, decls, this->arguments));

			assert(best_decl != nullptr);

			fn_type = TRY(best_decl->typecheck(cs));
			m_resolved_func_decl = best_decl;
		}
		else
		{
			fn_type = TRY(this->callee->typecheck(cs));
			m_resolved_func_decl = nullptr;
		}

		if(not fn_type->isFunction())
			error(this->location, "callee of function call must be a function type, got '{}'", fn_type);

		return Ok(fn_type->toFunction()->returnType());
	}

	ErrorOr<std::optional<Value>> FunctionCall::evaluate(Interpreter* cs) const
	{
		// TODO: maybe do this only once (instead of twice, once while typechecking and one for eval)
		// again, if this is an identifier, we do the separate thing.

		std::vector<Value> args {};

		if(auto ident = dynamic_cast<const Ident*>(this->callee.get()); ident != nullptr)
		{
			assert(m_resolved_func_decl != nullptr);

			auto decl = m_resolved_func_decl;
			auto [ordered_args, _] = TRY(resolve_argument_order<Value>(cs, decl, this->arguments,  //
				[cs](auto& arg) -> ErrorOr<Value> {
					if(auto value = TRY(arg.value->evaluate(cs)); value.has_value())
						return Ok(std::move(*value));

					return ErrFmt("expression did not yield a value");
				}));

			for(size_t i = 0; i < ordered_args.size(); i++)
			{
				if(auto it = ordered_args.find(i); it == ordered_args.end())
				{
					if(auto fdecl = dynamic_cast<FunctionDecl*>(decl); fdecl != nullptr)
					{
						auto& params = fdecl->params();
						if(params[i].default_value == nullptr)
							return ErrFmt("missing argument for parameter '{}'", params[i].name);

						auto tmp = TRY(params[i].default_value->evaluate(cs));
						if(not tmp.has_value())
							return ErrFmt("expression did not yield a value");

						args.push_back(std::move(*tmp));
					}
					else
					{
						return ErrFmt("????");
					}
				}
				else
				{
					args.push_back(std::move(it->second));
				}
			}

			auto func_defn = m_resolved_func_decl->resolved_defn;

			// check what kind of defn it is
			if(auto builtin = dynamic_cast<BuiltinFunctionDefn*>(func_defn); builtin != nullptr)
			{
				return builtin->function(cs, args);
			}
			else
			{
				return ErrFmt("not implemented");
			}
		}
		else
		{
			// TODO:
			return ErrFmt("not implemented");
		}

		// now we evaluate the lhs to get the function.
		// auto lhs = TRY(this->callee->evaluate(cs));
		// if(not lhs.has_value())
		// return ErrFmt("expression did not yield a value");
		// else if(not lhs->isFunction())
		// return ErrFmt("called expression was not a function");

		// fn_ptr = lhs->getFunction();
		// return Ok(fn_ptr(args));
	}
}