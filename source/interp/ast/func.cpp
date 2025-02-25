// func.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"         // for FunctionDefn, VariableDefn, Function...
#include "interp/type.h"        // for Type, FunctionType
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter, DefnTree, StackFrame
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp::ast
{
	static ErrorOr<cst::Declaration*> create_function_declaration(Typechecker* ts,
	    Location loc,
	    const std::string& name,
	    const std::vector<FunctionDefn::Param>& params,
	    frontend::PType return_type)
	{
		bool saw_variadic = false;

		std::vector<const Type*> param_types {};
		std::vector<cst::Declaration::FunctionDecl::Param> decl_params {};

		for(auto& param : params)
		{
			auto type = TRY(ts->resolveType(param.type));
			param_types.push_back(type);

			if(type->isVariadicArray())
			{
				if(saw_variadic)
					return ErrMsg(ts, "only one variadic array parameter is allowed per function");

				saw_variadic = true;
			}

			std::unique_ptr<cst::Expr> default_value;
			if(param.default_value != nullptr)
			{
				default_value = TRY(param.default_value->typecheck(ts)).take_expr();
				if(not ts->canImplicitlyConvert(default_value->type(), type))
				{
					return ErrMsg(ts,
					    "default value for parameter '{}' has type '{}', "
					    "which is incompatible with parameter type '{}'",
					    param.name, type, default_value->type());
				}
			}

			decl_params.push_back({
			    .name = param.name,
			    .type = type,
			    .is_mutable = param.is_mutable,
			    .default_value = std::move(default_value),
			});
		}

		auto fn_type = Type::makeFunction(std::move(param_types), TRY(ts->resolveType(return_type)));

		auto decl = cst::Declaration(loc, ts->current(), name, fn_type, /* mutable: */ false);
		decl.function_decl = cst::Declaration::FunctionDecl {
			.params = std::move(decl_params),
		};

		return ts->current()->declare(std::move(decl));
	}

	static ErrorOr<TCResult> typecheck_function_definition(Typechecker* ts,
	    const ast::FunctionDefn* func_defn,
	    cst::Declaration* decl)
	{
		assert(decl != nullptr);

		auto decl_type = decl->type;
		auto tree = ts->current()->lookupOrDeclareNamespace(zpr::sprint("{}.{}", decl->name, decl_type->str()));
		auto _ = ts->pushTree(tree);

		std::vector<cst::FunctionDefn::Param> new_params {};

		// make definitions for our parameters
		for(size_t i = 0; i < func_defn->params.size(); i++)
		{
			auto& param = func_defn->params[i];

			// make variadic arrays into non-variadic ones inside the function
			auto param_type = param.type;
			if(param_type.isVariadicArray())
				param_type = frontend::PType::array(param_type.getArrayElement());

			auto vdef = VariableDefn(param.loc, param.name, /* mutable: */ param.is_mutable, /* global: */ false,
			    /* init: */ nullptr, /* type: */ param_type);

			TRY(vdef.declare(ts));
			auto cst_vdef = TRY(vdef.typecheck(ts)).take<cst::VariableDefn>();

			new_params.push_back({
			    .defn = std::move(cst_vdef),
			    .default_value = decl->function_decl->params[i].default_value.get(),
			});
		}

		auto rt = decl_type->toFunction()->returnType();
		auto __ = ts->enterFunctionWithReturnType(rt);

		auto cst_body = TRY(func_defn->body->typecheck(ts)).take<cst::Block>();

		if(not rt->isVoid() && not func_defn->body->checkAllPathsReturn(rt))
			return ErrMsg(ts, "not all control paths return a value");

		auto defn = std::make_unique<cst::FunctionDefn>(func_defn->loc(), decl, std::move(new_params),
		    std::move(cst_body));

		decl->define(defn.get());
		return TCResult::ofVoid(std::move(defn));
	}

	ErrorOr<void> FunctionDefn::declare(Typechecker* ts) const
	{
		auto _ = ts->pushLocation(this->loc());

		if(this->generic_params.empty())
		{
			this->declaration = TRY(create_function_declaration(ts, m_location, this->name, this->params,
			    this->return_type));
			return Ok();
		}
		else
		{
			auto decl = cst::Declaration(this->loc(), ts->current(), this->name,
			    Type::makeFunction({}, Type::makeVoid()), /* mutable: */ false);

			decl.generic_func = this;

			this->declaration = TRY(ts->current()->declare(std::move(decl)));
			return Ok();
		}
	}

	ErrorOr<TCResult> FunctionDefn::instantiateGeneric(Typechecker* ts,
	    const util::hashmap<std::string, const Type*>& type_args) const
	{
		// declare the generic type args so they can be resolved.
		auto _ = ts->pushTypeArguments(type_args);

		// make a new declaration with the correct type for the function.
		auto inst_decl = TRY(create_function_declaration(ts, m_location, this->name, this->params, this->return_type));
		return typecheck_function_definition(ts, this, inst_decl);
	}

	ErrorOr<TCResult> FunctionDefn::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(this->declaration != nullptr);
		if(this->declaration->generic_func != nullptr)
		{
			// well we have to return *something*, and i don't want to throw nullptrs everywhere so
			return TCResult::ofVoid<cst::GenericPlaceholderDefn>(this->loc(), this->declaration);
		}

		return typecheck_function_definition(ts, this, this->declaration);
	}


	ErrorOr<void> BuiltinFunctionDefn::declare(Typechecker* ts) const
	{
		this->declaration = TRY(create_function_declaration(ts, m_location, this->name, this->params,
		    this->return_type));
		return Ok();
	}

	ErrorOr<TCResult> BuiltinFunctionDefn::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(this->declaration != nullptr);

		auto defn = std::make_unique<cst::BuiltinFunctionDefn>(m_location, this->declaration, this->function);
		this->declaration->define(defn.get());

		return TCResult::ofVoid(std::move(defn));
	}
}
