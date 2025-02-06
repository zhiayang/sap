// call.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "location.h"

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"
#include "interp/overload_resolution.h"

namespace sap::interp::ast
{
	static ErrorOr<ExpectedParams> convert_params(const Typechecker* ts, const FunctionType* fn_type)
	{
		ExpectedParams params {};

		for(size_t i = 0; i < fn_type->parameterTypes().size(); i++)
		{
			params.push_back({
			    .name = "",
			    .type = fn_type->parameterTypes()[i],
			    .default_value = nullptr,
			});
		}

		return OkMove(params);
	}

	static ErrorOr<ExpectedParams> convert_params(const Typechecker* ts, const cst::Declaration* decl)
	{
		if(decl->function_decl.has_value())
		{
			auto& fdecl = *decl->function_decl;
			auto decl_type = decl->type->toFunction();

			ExpectedParams params {};
			for(size_t i = 0; i < fdecl.params.size(); i++)
			{
				params.push_back({
				    .name = fdecl.params[i].name,
				    .type = decl_type->parameterTypes()[i],
				    .default_value = fdecl.params[i].default_value.get(),
				});
			}

			return OkMove(params);
		}
		else
		{
			if(not decl->type->isFunction())
				return ErrMsg(ts, "somehow we are calling a non-function type??? '{}'", decl->type->str());

			return convert_params(ts, decl->type->toFunction());
		}
	}

	using ResolvedOverloadSet = ErrorOr<std::pair<const cst::Declaration*, ArrangedArguments>>;

	static ErrorOr<ArrangedArguments> get_calling_cost(Typechecker* ts,
	    const cst::Declaration* decl,
	    const std::vector<InputArg>& arguments)
	{
		return arrangeCallArguments(ts, TRY(convert_params(ts, decl)), arguments, //
		    "function", "argument", "parameter");
	}

	static ResolvedOverloadSet resolve_overload_set(Typechecker* ts,
	    const std::vector<const cst::Declaration*>& decls,
	    const std::vector<InputArg>& arguments)
	{
		std::vector<const cst::Declaration*> best_decls {};
		std::vector<std::pair<const cst::Declaration*, ErrorMessage>> failed_decls {};

		int best_cost = INT_MAX;
		ArrangedArguments::ArgList arg_arrangement {};

		for(auto decl : decls)
		{
			auto result = get_calling_cost(ts, decl, arguments);
			if(result.is_err())
			{
				failed_decls.emplace_back(decl, result.take_error());
				continue;
			}

			auto arr = result.take_value();

			if(arr.coercion_cost == best_cost)
			{
				best_decls.push_back(decl);
			}
			else if(arr.coercion_cost < best_cost)
			{
				best_cost = arr.coercion_cost;
				best_decls = { decl };
				arg_arrangement = std::move(arr.arguments);
			}
		}

		if(best_decls.size() == 1)
		{
			return Ok<std::pair<const cst::Declaration*, ArrangedArguments>>(best_decls[0], std::move(arg_arrangement));
		}

		if(best_decls.empty())
		{
			std::string arg_types_str {};
			for(size_t i = 0; i < arguments.size(); i++)
			{
				if(i > 0)
					arg_types_str += ", ";

				if(arguments[i].type.has_value())
					arg_types_str += (*arguments[i].type)->str();
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

	static ErrorOr<std::vector<cst::ExprOrDefaultPtr>> create_final_arg_list(Typechecker* ts,
	    const Location& location,
	    bool is_rewritten_ufcs,
	    const ArrangedArguments& arg_arrangement,
	    std::vector<std::unique_ptr<cst::Expr>>& input_args,
	    const std::vector<FunctionCall::Arg>& call_args)
	{
		std::vector<cst::ExprOrDefaultPtr> final_args {};
		for(size_t i = 0; i < arg_arrangement.arguments.size(); i++)
		{
			auto do_one_arg = [&, cur_arg = i](const Type* param_type, const FinalArg::ArgIdxOrDefault& arg)
			    -> ErrorOr<cst::ExprOrDefaultPtr> {
				if(arg.is_right())
					return Ok(arg.right());

				size_t arg_idx = arg.left();
				if(input_args[arg_idx] != nullptr)
				{
					return Ok(std::move(input_args[arg_idx]));
				}
				else
				{
					bool is_ufcs_self = is_rewritten_ufcs && cur_arg == 0;
					auto expr = TRY(call_args[arg_idx].value->typecheck(ts, /* infer: */ param_type,
					    /* keep_lvalue: */ is_ufcs_self));

					return Ok(std::move(expr).take_expr());
				}
			};

			auto& arg = arg_arrangement.arguments[i];
			if(arg.value.is_left())
			{
				final_args.push_back(TRY(do_one_arg(arg.param_type, arg.value.left())));
			}
			else
			{
				// variadic pack.
				assert(arg.param_type->isVariadicArray());

				std::vector<cst::ExprOrDefaultPtr> pack {};
				for(auto& e : arg.value.right())
					pack.push_back(TRY(do_one_arg(arg.param_type->arrayElement(), e)));

				final_args.emplace_back(std::make_unique<cst::VariadicPackExpr>(location, arg.param_type,
				    std::move(pack)));
			}
		}

		return OkMove(final_args);
	}



	ErrorOr<TCResult> FunctionCall::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		std::vector<InputArg> processed_arg_types {};
		std::vector<std::unique_ptr<cst::Expr>> processed_args {};

		bool ufcs_is_rvalue = false;
		bool ufcs_is_mutable = false;
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
				processed_args.push_back({});
				processed_arg_types.push_back({
				    .type = std::nullopt,
				    .name = arg.name,
				});
				continue;
			}

			bool is_ufcs_self = this->rewritten_ufcs && i == 0;

			auto tc_arg = TRY(arg.value->typecheck(ts, /* infer: */ nullptr, /* keep_lvalue: */ is_ufcs_self));
			const Type* ty = tc_arg.type();

			if(is_ufcs_self)
			{
				// we can yeet rvalues into mutability
				if(tc_arg.isMutable())
					ty = ty->mutablePointerTo(), ufcs_is_mutable = true;
				else if(not tc_arg.isLValue())
					ufcs_is_rvalue = true;
				else
					ty = ty->pointerTo();
			}

			processed_args.push_back(std::move(tc_arg).take_expr());
			processed_arg_types.push_back({
			    .type = ty,
			    .name = arg.name,
			});
		}

		using UFCSKind = cst::FunctionCall::UFCSKind;

		// if the function expression is an identifier, we resolve it manually to handle overloading.
		if(auto ident = dynamic_cast<const Ident*>(this->callee.get()); ident != nullptr)
		{
			const DefnTree* lookup_in = ts->current();
			if(this->rewritten_ufcs)
			{
				// the first guy cannot be deferred, obviously.
				assert(processed_args[0] != nullptr);
				lookup_in = TRY(ts->getDefnTreeForType(processed_args[0]->type()));
			}

			auto decls = TRY(lookup_in->lookupRecursive(ident->name));
			assert(decls.size() > 0);

			auto [maybe_best, ufcs_kind] = ([&]() -> std::pair<ResolvedOverloadSet, UFCSKind> {
				// note that `processed_arg_types` already does the appropriate transformation
				// for ufcs (in terms of making pointers when appropriate).
				auto ret = resolve_overload_set(ts, decls, processed_arg_types);
				if(ret.ok())
				{
					auto u = UFCSKind::None;
					if(this->rewritten_ufcs)
					{
						if(ufcs_is_rvalue)
							u = UFCSKind::ByValue;
						else if(ufcs_is_mutable)
							u = UFCSKind::MutablePointer;
						else
							u = UFCSKind::ConstPointer;
					}

					return { OkMove(ret.unwrap()), u };
				}

				if(not ufcs_is_rvalue)
					return { Err(std::move(ret.error())), UFCSKind::None };

				// if the ufcs was an rvalue, then we would have tried the pass-self-by-value
				// overload first; now, try the self-by-pointer overload (if it exists).
				processed_arg_types[0].type = (*processed_arg_types[0].type)->mutablePointerTo();

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

			auto fn_type = best_decl->type->toFunction();
			auto final_callee = best_decl;

			auto final_args = TRY(create_final_arg_list(ts, m_location, this->rewritten_ufcs, arg_arrangement,
			    processed_args, this->arguments));

			if(not fn_type->isFunction())
				return ErrMsg(ts, "callee of function call must be a function type, got '{}'", fn_type->str());

			return TCResult::ofRValue<cst::FunctionCall>(m_location, //
			    fn_type->returnType(),                               //
			    ufcs_kind,                                           //
			    std::move(final_args),                               //
			    final_callee);
		}
		else
		{
			// if this is an expression, we only have the parameter types to go on
			// (function types do not contain names), so resolution is (theoretically) simpler.
			// unfortunately a lot of the code that we have above is written for resolving,
			// which means we basically need separate code for doing this. anyway it should
			// be a bit simpler since there's no overloading or anything -- just need to handle ufcs.

			auto ufcs_kind = UFCSKind::None;
			auto final_callee = TRY(this->callee->typecheck(ts)).take_expr();

			if(const auto* pso = dynamic_cast<const cst::PartiallyResolvedOverloadSet*>(final_callee.get()))
			{
				return ErrMsg(this->loc(), "a");
			}

			if(not final_callee->type()->isFunction())
			{
				return ErrMsg(ts, "callee of function call must be a function type, got '{}'",
				    final_callee->type()->str());
			}

			auto fn_type = final_callee->type()->toFunction();

			ArrangedArguments arranged {};
			auto arranged_1 = arrangeCallArguments(ts, TRY(convert_params(ts, final_callee->type()->toFunction())),
			    processed_arg_types, "function", "argument", "parameter");

			if(arranged_1.ok())
			{
				arranged = TRY(std::move(arranged_1));
				if(this->rewritten_ufcs)
				{
					if(ufcs_is_rvalue)
						ufcs_kind = UFCSKind::ByValue;
					else if(ufcs_is_mutable)
						ufcs_kind = UFCSKind::MutablePointer;
					else
						ufcs_kind = UFCSKind::ConstPointer;
				}
			}
			else
			{
				// if the ufcs expr is not an rvalue, there's no point trying this second configuration.
				if(not ufcs_is_rvalue)
					return Err(arranged_1.take_error());

				processed_arg_types[0].type = (*processed_arg_types[0].type)->mutablePointerTo();

				// try again
				arranged = TRY(arrangeCallArguments(ts, TRY(convert_params(ts, final_callee->type()->toFunction())),
				    processed_arg_types, "function", "argument", "parameter"));

				ufcs_kind = ufcs_is_rvalue ? UFCSKind::MutablePointer : UFCSKind::ConstPointer;
			}

			auto final_args = TRY(create_final_arg_list(ts, m_location, this->rewritten_ufcs, arranged, processed_args,
			    this->arguments));

			return TCResult::ofRValue<cst::FunctionCall>(m_location, //
			    fn_type->returnType(),                               //
			    ufcs_kind,                                           //
			    std::move(final_args),                               //
			    std::move(final_callee));
		}
	}
}
