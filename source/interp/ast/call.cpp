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

namespace sap::interp::ast
{
	static ErrorOr<std::vector<std::tuple<std::string, const Type*, const cst::Expr*>>>
	convert_params(const Typechecker* ts, const cst::Declaration* decl)
	{
		if(decl->function_decl.has_value())
		{
			auto& fdecl = *decl->function_decl;
			auto decl_type = decl->type->toFunction();

			std::vector<std::tuple<std::string, const Type*, const cst::Expr*>> params {};
			for(size_t i = 0; i < fdecl.params.size(); i++)
			{
				params.push_back({
				    fdecl.params[i].name,
				    decl_type->parameterTypes()[i],
				    fdecl.params[i].default_value.get(),
				});
			}

			return Ok(std::move(params));
		}
		else
		{
			return ErrMsg(ts, "?!");
		}
	}

	using ResolvedOverloadSet = ErrorOr<std::pair<const cst::Declaration*, ArrangedArguments<const Type*>>>;

	static ErrorOr<std::pair<int, ArrangedArguments<const Type*>>> get_calling_cost(Typechecker* ts,
	    const cst::Declaration* decl,
	    const std::vector<ArrangeArg<const Type*>>& arguments)
	{
		auto params = TRY(convert_params(ts, decl));
		auto ordered = TRY(arrangeArgumentTypes(ts, params, arguments, //
		    "function", "argument", "argument for parameter"));

		auto cost = TRY(getCallingCost(ts, params, ordered.param_idx_to_args, "function", "argument",
		    "argument for parameter"));

		using X = std::pair<int, ArrangedArguments<const Type*>>;
		return Ok<X>(cost, std::move(ordered));
	}

	static ResolvedOverloadSet resolve_overload_set(Typechecker* ts,
	    const std::vector<const cst::Declaration*>& decls,
	    const std::vector<ArrangeArg<const Type*>>& arguments)
	{
		std::vector<const cst::Declaration*> best_decls {};
		std::vector<std::pair<const cst::Declaration*, ErrorMessage>> failed_decls {};

		int best_cost = INT_MAX;

		ArrangedArguments<const Type*> arg_arrangement {};

		for(auto decl : decls)
		{
			auto result = get_calling_cost(ts, decl, arguments);

			if(result.is_err())
			{
				failed_decls.emplace_back(decl, result.take_error());
				continue;
			}

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
			return Ok<std::pair<const cst::Declaration*, ArrangedArguments<const Type*>>>(best_decls[0],
			    std::move(arg_arrangement));
		}

		if(best_decls.empty())
		{
			std::string arg_types_str {};
			for(size_t i = 0; i < arguments.size(); i++)
			{
				if(i > 0)
					arg_types_str += ", ";

				if(arguments[i].value != nullptr)
					arg_types_str += arguments[i].value->str();
				else
					arg_types_str += "<unknown>";
			}

			auto err = ErrorMessage(ts,
			    zpr::sprint("no matching function for call matching arguments ({}) among {} candidate{}", //
			        arg_types_str, failed_decls.size(), failed_decls.size() == 1 ? "" : "s"));

			for(auto& [decl, msg] : failed_decls)
				err.addInfo(decl->location, msg.string());

			return Err(std::move(err));
		}

		return ErrMsg(ts, "ambiguous call: {} candidates", best_decls.size());
	}


	ErrorOr<TCResult2> FunctionCall::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		std::vector<ArrangeArg<const Type*>> processed_arg_types {};
		std::vector<ArrangeArg<std::unique_ptr<cst::Expr>>> processed_args {};

		bool ufcs_is_rvalue = false;
		for(size_t i = 0; i < this->arguments.size(); i++)
		{
			auto& arg = this->arguments[i];

			static constexpr bool (*check_must_defer)(const Expr*) = [](const Expr* x) -> bool {
				if(auto str_lit = dynamic_cast<const StructLit*>(x); str_lit && str_lit->is_anonymous)
				{
					return true;
				}
				else if(auto ctx_lit = dynamic_cast<const ContextIdent*>(x); ctx_lit)
				{
					return true;
				}
				else if(auto arr = dynamic_cast<const ArrayLit*>(x); arr && not arr->elem_type.has_value())
				{
					for(auto& e : arr->elements)
					{
						if(check_must_defer(e.get()))
							return true;
					}

					return false;
				}

				return false;
			};

			bool must_defer = check_must_defer(arg.value.get());

			if(must_defer)
			{
				processed_args.push_back({
				    .value = nullptr,
				    .name = arg.name,
				});

				processed_arg_types.push_back({
				    .value = nullptr,
				    .name = arg.name,
				});
				continue;
			}

			bool is_ufcs_self = this->rewritten_ufcs && i == 0;

			auto tc_arg = TRY(arg.value->typecheck2(ts, /* infer: */ nullptr, /* keep_lvalue: */ is_ufcs_self));
			const Type* ty = tc_arg.type();

			if(is_ufcs_self)
			{
				// we can yeet rvalues into mutability
				if(tc_arg.isMutable())
					ty = ty->mutablePointerTo();
				else if(not tc_arg.isLValue())
					ufcs_is_rvalue = true;
				else
					ty = ty->pointerTo();
			}

			processed_args.push_back({
			    .value = std::move(tc_arg).take_expr(),
			    .name = arg.name,
			});

			processed_arg_types.push_back({
			    .value = processed_args.back().value->type(),
			    .name = arg.name,
			});
		}

		using UFCSKind = cst::FunctionCall::UFCSKind;

		auto ufcs_kind = UFCSKind::None;
		const FunctionType* fn_type = nullptr;
		const cst::Declaration* final_callee = nullptr;
		std::vector<std::unique_ptr<cst::Expr>> final_args {};

		// if the function expression is an identifier, we resolve it manually to handle overloading.
		if(auto ident = dynamic_cast<const Ident*>(this->callee.get()); ident != nullptr)
		{
			const DefnTree* lookup_in = ts->current();
			if(this->rewritten_ufcs)
			{
				// the first guy cannot be deferred, obviously.
				assert(processed_args[0].value != nullptr);
				lookup_in = TRY(ts->getDefnTreeForType(processed_args[0].value->type()));
			}

			auto decls = TRY(lookup_in->lookupRecursive(ident->name));
			assert(decls.size() > 0);

			auto [maybe_best, _ufcs_kind] = ([&]() -> std::pair<ResolvedOverloadSet, UFCSKind> {
				// first, try with the normal set.
				auto ret = resolve_overload_set(ts, decls, processed_arg_types);

				if(ret.ok())
				{
					return {
						Ok(std::move(ret.unwrap())),
						(this->rewritten_ufcs && ufcs_is_rvalue) ? UFCSKind::ByValue : UFCSKind::None,
					};
				}

				// we were not ok.
				if(not ufcs_is_rvalue)
					return { Err(std::move(ret.error())), UFCSKind::None };

				// if the ufcs was an rvalue, then we would have tried the pass-self-by-value
				// overload first; now, try the self-by-pointer overload (if it exists).
				processed_arg_types[0].value = processed_arg_types[0].value->mutablePointerTo();

				ret = resolve_overload_set(ts, decls, processed_arg_types);
				if(not ret.ok())
					return { Err(ret.take_error()), UFCSKind::None };

				return {
					Ok(ret.take_value()),
					ufcs_is_rvalue ? UFCSKind::MutablePointer : UFCSKind::ConstPointer,
				};
			}());

			auto [best_decl, arg_arrangement] = TRY(std::move(maybe_best));
			assert(best_decl != nullptr);

			fn_type = best_decl->type->toFunction();
			final_callee = best_decl;
			ufcs_kind = _ufcs_kind;

			// now that we have the best decl, typecheck any arguments that are deferred.
			for(size_t i = 0; i < processed_args.size(); i++)
			{
				if(processed_args[i].value != nullptr)
					continue;

				auto param_idx = arg_arrangement.arg_idx_to_param_idx[i];
				auto param_type = fn_type->toFunction()->parameterTypes()[param_idx];

				bool is_ufcs_self = this->rewritten_ufcs && i == 0;
				if(is_ufcs_self && (ufcs_kind != UFCSKind::ByValue))
					param_type = param_type->pointerElement();

				auto expr = TRY(this->arguments[i].value->typecheck2(ts, /* infer: */ param_type, /* keep_lvalue: */
				    is_ufcs_self));

				processed_args[i].value = std::move(expr).take_expr();
			}

			// now rearrange all the args according to the thing.
			util::hashmap<size_t, std::unique_ptr<cst::Expr>> ordered_args {};
			for(size_t i = 0; i < processed_args.size(); i++)
			{
				auto param_idx = arg_arrangement.arg_idx_to_param_idx[i];

				assert(not ordered_args.contains(param_idx));
				ordered_args[param_idx] = std::move(processed_args[i].value);
			}

			for(size_t i = 0; i < ordered_args.size(); i++)
			{
				assert(ordered_args.contains(i));
				final_args.push_back(std::move(ordered_args[i]));
			}
		}
		else
		{
			return ErrMsg(ts, "expr call not supported yet");
		}

		if(not fn_type->isFunction())
			return ErrMsg(ts, "callee of function call must be a function type, got '{}'", fn_type->str());

		return TCResult2::ofRValue<cst::FunctionCall>(m_location, //
		    fn_type->returnType(),                                //
		    ufcs_kind,                                            //
		    std::move(final_args),                                //
		    final_callee);
	}
}
