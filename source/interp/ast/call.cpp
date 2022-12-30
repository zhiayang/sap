// call.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "location.h" // for error

#include "interp/ast.h"         // for Declaration, FunctionCall, Expr, Fun...
#include "interp/type.h"        // for Type, FunctionType
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter, DefnTree
#include "interp/eval_result.h" // for EvalResult, TRY_VALUE

namespace sap::interp
{
	template <typename T, typename ArgProcessor>
	ErrorOr<std::pair<std::unordered_map<size_t, T>, std::vector<const Type*>>> resolve_argument_order(Interpreter* cs,
	    const Declaration* decl, const std::vector<FunctionCall::Arg>& args, ArgProcessor&& ap)
	{
		// TODO: check for variables.
		if(auto fdecl = dynamic_cast<const FunctionDecl*>(decl); fdecl != nullptr)
		{
			auto decl_type = fdecl->get_type()->toFunction();
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
				decl_param_types.push_back(decl_type->parameterTypes()[i]);
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

	static ErrorOr<int> get_calling_cost(Interpreter* cs, const Declaration* decl,
	    const std::vector<FunctionCall::Arg>& arguments)
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
				if(auto fdecl = dynamic_cast<const FunctionDecl*>(decl); fdecl != nullptr)
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

			// if the param is an any, we can just do it, but with extra cost.
			if(auto param_type = decl_params_types[i]; arg_type != param_type)
			{
				if(param_type->isAny())
				{
					cost += 2;
				}
				else
				{
					return ErrFmt("mismatched types for argument {}: got '{}', expected '{}'", //
					    1 + i, arg_type, param_type);
				}
			}
			else
			{
				cost += 1;
			}
		}

		return Ok(cost);
	}


	static ErrorOr<const Declaration*> resolve_overload_set(Interpreter* cs, const std::vector<const Declaration*>& decls,
	    const std::vector<FunctionCall::Arg>& arguments)
	{
		std::vector<const Declaration*> best_decls {};
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

		// TODO: this might be problematic if we want to have bidirectional type inference
		// either way, we must ensure that all the arguments typecheck to begin with
		for(auto& arg : this->arguments)
			TRY(arg.value->typecheck(cs));

		// if the lhs is an identifier, we resolve it manually to handle overloading.
		if(auto ident = dynamic_cast<const Ident*>(this->callee.get()); ident != nullptr)
		{
			auto decls = TRY(cs->current()->lookup(ident->name));
			assert(decls.size() > 0);

			const Declaration* best_decl = TRY(resolve_overload_set(cs, decls, this->arguments));
			assert(best_decl != nullptr);

			fn_type = TRY(best_decl->typecheck(cs));
			m_resolved_func_decl = best_decl;
		}
		else
		{
			zpr::println("warning: expr call");

			fn_type = TRY(this->callee->typecheck(cs));
			m_resolved_func_decl = nullptr;
		}

		if(not fn_type->isFunction())
			error(this->location, "callee of function call must be a function type, got '{}'", fn_type);

		return Ok(fn_type->toFunction()->returnType());
	}

	ErrorOr<EvalResult> FunctionCall::evaluate(Interpreter* cs) const
	{
		// TODO: maybe do this only once (instead of twice, once while typechecking and one for eval)
		// again, if this is an identifier, we do the separate thing.

		std::vector<Value> args {};

		if(auto ident = dynamic_cast<const Ident*>(this->callee.get()); ident != nullptr)
		{
			assert(m_resolved_func_decl != nullptr);

			auto decl = m_resolved_func_decl;
			auto [ordered_args, _] = TRY(resolve_argument_order<Value>(cs, decl, this->arguments, //
			    [cs](auto& arg) -> ErrorOr<Value> {
				    return Ok(std::move(TRY_VALUE(arg.value->evaluate(cs))));
			    }));

			for(size_t i = 0; i < ordered_args.size(); i++)
			{
				if(auto it = ordered_args.find(i); it == ordered_args.end())
				{
					if(auto fdecl = dynamic_cast<const FunctionDecl*>(decl); fdecl != nullptr)
					{
						auto& params = fdecl->params();
						if(params[i].default_value == nullptr)
							return ErrFmt("missing argument for parameter '{}'", params[i].name);

						auto tmp = TRY_VALUE(params[i].default_value->evaluate(cs));
						args.push_back(std::move(tmp));
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

			auto the_defn = m_resolved_func_decl->resolved_defn;

			// check what kind of defn it is
			if(auto builtin_defn = dynamic_cast<const BuiltinFunctionDefn*>(the_defn); builtin_defn != nullptr)
			{
				return builtin_defn->function(cs, args);
			}
			else if(auto func_defn = dynamic_cast<const FunctionDefn*>(the_defn); func_defn != nullptr)
			{
				return func_defn->call(cs, args);
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
