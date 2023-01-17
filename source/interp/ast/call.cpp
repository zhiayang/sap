// call.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "location.h" // for error

#include "interp/ast.h" // for Declaration, FunctionCall, Expr, Fun...
#include "interp/misc.h"
#include "interp/type.h"        // for Type, FunctionType
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter, DefnTree
#include "interp/eval_result.h" // for EvalResult, TRY_VALUE

namespace sap::interp
{
	template <typename TsEv>
	static ErrorOr<std::vector<std::tuple<std::string, const Type*, const Expr*>>> convert_params(const TsEv* ts_ev,
	    const Declaration* decl)
	{
		if(auto fdecl = dynamic_cast<const FunctionDecl*>(decl); fdecl != nullptr)
		{
			auto decl_type = fdecl->get_type()->toFunction();
			auto& decl_params = fdecl->params();

			std::vector<std::tuple<std::string, const Type*, const Expr*>> params {};
			for(size_t i = 0; i < decl_params.size(); i++)
			{
				params.push_back({
				    decl_params[i].name,
				    decl_type->parameterTypes()[i],
				    decl_params[i].default_value.get(),
				});
			}

			return Ok(std::move(params));
		}
		else
		{
			return ErrMsg(ts_ev, "?!");
		}
	}

	static ErrorOr<int> get_calling_cost(Typechecker* ts,
	    const Declaration* decl,
	    const std::vector<ArrangeArg<const Type*>>& arguments)
	{
		auto params = TRY(convert_params(ts, decl));
		auto ordered = TRY(arrange_argument_types(ts, params, arguments, //
		    "function", "argument", "argument for parameter"));

		return get_calling_cost(ts, params, ordered, "function", "argument", "argument for parameter");
	}


	static ErrorOr<const Declaration*> resolve_overload_set(Typechecker* ts,
	    const std::vector<const Declaration*>& decls,
	    const std::vector<ArrangeArg<const Type*>>& arguments)
	{
		std::vector<const Declaration*> best_decls {};
		int best_cost = INT_MAX;

		for(auto decl : decls)
		{
			auto cost = get_calling_cost(ts, decl, arguments);

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
			return ErrMsg(ts, "no matching function for call");

		return ErrMsg(ts, "ambiguous call");
	}



	ErrorOr<TCResult> FunctionCall::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		std::vector<ArrangeArg<const Type*>> processed_args {};

		// TODO: this might be problematic if we want to have bidirectional type inference
		// either way, we must ensure that all the arguments typecheck to begin with

		for(size_t i = 0; i < this->arguments.size(); i++)
		{
			auto& arg = this->arguments[i];

			auto t = TRY(arg.value->typecheck(ts));
			const Type* ty = t.type();

			if(this->rewritten_ufcs && i == 0)
			{
				m_ufcs_self_is_mutable = t.isMutable();
				if(t.isMutable())
					ty = ty->mutablePointerTo();
				else
					ty = ty->pointerTo();
			}

			processed_args.push_back({
			    .name = arg.name,
			    .value = ty,
			});
		}

		const Type* fn_type = nullptr;

		// if the function expression is an identifier, we resolve it manually to handle overloading.
		if(auto ident = dynamic_cast<const Ident*>(this->callee.get()); ident != nullptr)
		{
			const DefnTree* lookup_in = ts->current();
			if(this->rewritten_ufcs)
			{
				lookup_in = TRY(ts->getDefnTreeForType(processed_args[0].value));
			}

			auto decls = TRY(lookup_in->lookup(ident->name));
			assert(decls.size() > 0);

			const Declaration* best_decl = TRY(resolve_overload_set(ts, decls, processed_args));
			assert(best_decl != nullptr);

			fn_type = TRY(best_decl->typecheck(ts)).type();
			m_resolved_func_decl = best_decl;
		}
		else
		{
			zpr::println("warning: expr call");

			fn_type = TRY(this->callee->typecheck(ts)).type();
			m_resolved_func_decl = nullptr;
		}

		if(not fn_type->isFunction())
			return ErrMsg(ts, "callee of function call must be a function type, got '{}'", fn_type);

		return TCResult::ofRValue(fn_type->toFunction()->returnType());
	}


	ErrorOr<EvalResult> FunctionCall::evaluate_impl(Evaluator* ev) const
	{
		// TODO: maybe do this only once (instead of twice, once while typechecking and one for eval)
		// again, if this is an identifier, we do the separate thing.

		std::vector<ArrangeArg<Value>> processed_args {};
		for(size_t i = 0; i < this->arguments.size(); i++)
		{
			auto& arg = this->arguments[i];
			auto val = TRY(arg.value->evaluate(ev));

			if(this->rewritten_ufcs)
			{
				Value* ptr = nullptr;
				if(val.isLValue())
					ptr = &val.get();
				else
					ptr = ev->frame().createTemporary(std::move(val).take());

				Value self {};
				if(m_ufcs_self_is_mutable)
					self = Value::mutablePointer(ptr->type(), ptr);
				else
					self = Value::pointer(ptr->type(), ptr);

				processed_args.push_back({
				    .name = arg.name,
				    .value = std::move(self),
				});
			}
			else
			{
				processed_args.push_back({
				    .name = arg.name,
				    .value = std::move(val).take(),
				});
			}
		}


		std::vector<Value> final_args {};
		auto _ = ev->pushCallFrame();

		if(auto ident = dynamic_cast<const Ident*>(this->callee.get()); ident != nullptr)
		{
			assert(m_resolved_func_decl != nullptr);

			auto decl = m_resolved_func_decl;
			auto params = TRY(convert_params(ev, decl));

			auto ordered_args = TRY(arrange_argument_values(ev, params, std::move(processed_args), "function", "argument",
			    "argument for parameter"));

			for(size_t i = 0; i < params.size(); i++)
			{
				auto param_type = std::get<1>(params[i]);
				if(auto it = ordered_args.find(i); it == ordered_args.end())
				{
					if(std::get<2>(params[i]) == nullptr)
						return ErrMsg(ev, "missing argument for parameter '{}'", std::get<0>(params[i]));

					auto tmp = TRY_VALUE(std::get<2>(params[i])->evaluate(ev));
					final_args.push_back(ev->castValue(std::move(tmp), param_type));
				}
				else
				{
					final_args.push_back(ev->castValue(std::move(it->second), param_type));
				}
			}

			auto the_defn = m_resolved_func_decl->definition();

			// check what kind of defn it is
			if(auto builtin_defn = dynamic_cast<const BuiltinFunctionDefn*>(the_defn); builtin_defn != nullptr)
			{
				return builtin_defn->function(ev, final_args);
			}
			else if(auto func_defn = dynamic_cast<const FunctionDefn*>(the_defn); func_defn != nullptr)
			{
				return func_defn->call(ev, final_args);
			}
			else
			{
				return ErrMsg(ev, "not implemented");
			}
		}
		else
		{
			// TODO:
			return ErrMsg(ev, "not implemented");
		}
	}
}
