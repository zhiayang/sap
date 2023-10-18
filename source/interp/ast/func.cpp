// func.cpp
// Copyright (c) 2022, zhiayang
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
				default_value = TRY(param.default_value->typecheck2(ts)).take_expr();
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

	ErrorOr<void> FunctionDefn::declare(Typechecker* ts) const
	{
		this->declaration = TRY(create_function_declaration(ts, m_location, this->name, this->params,
		    this->return_type));
		return Ok();
	}

	ErrorOr<TCResult2> FunctionDefn::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(this->declaration != nullptr);
		auto decl_type = this->declaration->type;

		// TODO: maybe a less weird mangling solution? idk
		auto tree = ts->current()->lookupOrDeclareNamespace(zpr::sprint("{}${}", this->declaration->name,
		    decl_type->str()));
		auto _ = ts->pushTree(tree);

		std::vector<cst::FunctionDefn::Param> new_params {};

		// make definitions for our parameters
		for(size_t i = 0; i < this->params.size(); i++)
		{
			auto& param = this->params[i];

			// make variadic arrays into non-variadic ones inside the function
			auto param_type = param.type;
			if(param_type.isVariadicArray())
				param_type = frontend::PType::array(param_type.getArrayElement());

			auto vdef = VariableDefn(param.loc, param.name, /* mutable: */ false, /* global: */ false,
			    /* init: */ nullptr, /* type: */ param_type);

			TRY(vdef.declare(ts));
			auto cst_vdef = TRY(vdef.typecheck2(ts)).take<cst::VariableDefn>();

			new_params.push_back({
			    .defn = std::move(cst_vdef),
			    .default_value = this->declaration->function_decl->params[i].default_value.get(),
			});
		}

		auto rt = decl_type->toFunction()->returnType();
		auto __ = ts->enterFunctionWithReturnType(rt);

		auto cst_body = TRY(this->body->typecheck2(ts)).take<cst::Block>();

		if(not rt->isVoid())
		{
			if(not this->body->checkAllPathsReturn(rt))
				return ErrMsg(ts, "not all control paths return a value");
		}

		return TCResult2::ofVoid<cst::FunctionDefn>(m_location, this->declaration, std::move(new_params),
		    std::move(cst_body));
	}


	ErrorOr<void> BuiltinFunctionDefn::declare(Typechecker* ts) const
	{
		this->declaration = TRY(create_function_declaration(ts, m_location, this->name, this->params,
		    this->return_type));
		return Ok();
	}

	ErrorOr<TCResult2> BuiltinFunctionDefn::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(this->declaration != nullptr);
		return TCResult2::ofVoid<cst::BuiltinFunctionDefn>(m_location, this->declaration, this->function);
	}
}
