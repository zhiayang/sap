// call.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "location.h" // for error

#include "interp/ast.h"         // for Declaration, FunctionCall, Expr, Fun...
#include "interp/type.h"        // for Type, FunctionType
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter, DefnTree
#include "interp/eval_result.h" // for EvalResult, TRY_VALUE
#include "interp/overload_resolution.h"

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

	static ErrorOr<std::pair<int, ArrangedArguments<const Type*>>> get_calling_cost(Typechecker* ts,
		const Declaration* decl,
		const std::vector<ArrangeArg<const Type*>>& arguments)
	{
		auto params = TRY(convert_params(ts, decl));
		auto ordered = TRY(arrangeArgumentTypes(ts, params, arguments, //
			"function", "argument", "argument for parameter"));

		auto cost = TRY(getCallingCost(ts, params, ordered.param_idx_to_arg, "function", "argument",
			"argument for parameter"));

		using X = std::pair<int, ArrangedArguments<const Type*>>;
		return Ok<X>(cost, std::move(ordered));
	}


	static ErrorOr<std::pair<const Declaration*, ArrangedArguments<const Type*>>> resolve_overload_set(Typechecker* ts,
		const std::vector<const Declaration*>& decls,
		const std::vector<ArrangeArg<const Type*>>& arguments)
	{
		std::vector<const Declaration*> best_decls {};
		int best_cost = INT_MAX;

		ArrangedArguments<const Type*> arg_arrangement {};

		for(auto decl : decls)
		{
			auto result = get_calling_cost(ts, decl, arguments);

			if(result.is_err())
				continue;

			auto [cost, ord] = result.take_value();

			if(cost == best_cost)
			{
				best_decls.push_back(decl);
			}
			else if(cost < best_cost)
			{
				best_cost = cost;
				best_decls = { decl };
				arg_arrangement = std::move(ord);
			}
		}

		if(best_decls.size() == 1)
		{
			return Ok<std::pair<const Declaration*, ArrangedArguments<const Type*>>>(best_decls[0],
				std::move(arg_arrangement));
		}

		if(best_decls.empty())
			return ErrMsg(ts, "no matching function for call");

		return ErrMsg(ts, "ambiguous call");
	}



	ErrorOr<TCResult> FunctionCall::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		std::vector<ArrangeArg<const Type*>> processed_args {};

		for(size_t i = 0; i < this->arguments.size(); i++)
		{
			auto& arg = this->arguments[i];

			bool must_defer = [&arg]() {
				auto* x = arg.value.get();
				if(auto str_lit = dynamic_cast<const StructLit*>(x); str_lit && str_lit->is_anonymous)
					return true;
				else if(auto enum_lit = dynamic_cast<const EnumLit*>(x); enum_lit)
					return true;

				return false;
			}();

			if(must_defer)
			{
				processed_args.push_back({
					.name = arg.name,
					.value = Type::makeVoid(),
					.deferred_typecheck = true,
				});
				continue;
			}

			bool is_ufcs_self = this->rewritten_ufcs && i == 0;

			auto t = TRY(arg.value->typecheck(ts, /* infer: */ nullptr, /* keep_lvalue: */ is_ufcs_self));
			const Type* ty = t.type();

			if(is_ufcs_self)
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
				lookup_in = TRY(ts->getDefnTreeForType(processed_args[0].value));

			auto decls = TRY(lookup_in->lookup(ident->name));
			assert(decls.size() > 0);

			auto [best_decl, arg_arrangement] = TRY(resolve_overload_set(ts, decls, processed_args));
			assert(best_decl != nullptr);

			fn_type = TRY(best_decl->typecheck(ts)).type();
			m_resolved_func_decl = best_decl;

			// now that we have the best decl, typecheck any arguments that are deferred.
			for(size_t i = 0; i < processed_args.size(); i++)
			{
				if(not processed_args[i].deferred_typecheck)
					continue;

				auto param_idx = arg_arrangement.arg_idx_to_param_idx[i];
				auto param_type = fn_type->toFunction()->parameterTypes()[param_idx];

				bool is_ufcs_self = this->rewritten_ufcs && i == 0;
				if(is_ufcs_self)
					param_type = param_type->pointerElement();

				TRY(this->arguments[i].value->typecheck(ts, /* infer: */ param_type, /* keep_lvalue: */
					is_ufcs_self));
			}
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

			if(i == 0 && this->rewritten_ufcs)
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

			auto arg_arrangement = TRY(arrangeArgumentValues(ev, params, std::move(processed_args), "function",
				"argument", "argument for parameter"));

			auto& ordered_args = arg_arrangement.param_idx_to_arg;

			bool is_variadic = not params.empty() && std::get<1>(params.back())->isVariadicArray();

			for(size_t i = 0; i < params.size(); i++)
			{
				auto param_type = std::get<1>(params[i]);
				if(i + 1 == params.size() && is_variadic)
				{
					auto variadic_elm = param_type->arrayElement();

					std::vector<Value> vararg_array {};
					for(size_t k = i;; k++)
					{
						if(auto it = ordered_args.find(k); it != ordered_args.end())
						{
							auto value = std::move(it->second);

							// if the value itself is variadic, it means it's a spread op
							if(value.value.type()->isVariadicArray())
							{
								auto arr = std::move(value.value).takeArray();
								for(auto& val : arr)
									vararg_array.push_back(ev->castValue(std::move(val), variadic_elm));
							}
							else
							{
								vararg_array.push_back(ev->castValue(std::move(value.value), variadic_elm));
							}
						}
						else
						{
							break;
						}
					}

					final_args.push_back(Value::array(param_type->arrayElement(), std::move(vararg_array)));
					break;
				}

				if(auto it = ordered_args.find(i); it != ordered_args.end())
				{
					final_args.push_back(ev->castValue(std::move(it->second.value), param_type));
				}
				else
				{
					if(std::get<2>(params[i]) == nullptr)
						return ErrMsg(ev, "missing argument for parameter '{}'", std::get<0>(params[i]));

					auto tmp = TRY_VALUE(std::get<2>(params[i])->evaluate(ev));
					final_args.push_back(ev->castValue(std::move(tmp), param_type));
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
