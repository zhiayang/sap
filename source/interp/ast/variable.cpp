// variable.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"         // for VariableDefn, Expr, Declaration, Var...
#include "interp/type.h"        // for Type
#include "interp/interp.h"      // for Interpreter, DefnTree, StackFrame
#include "interp/eval_result.h" // for EvalResult, TRY_VALUE

namespace sap::interp::ast
{
	ErrorOr<void> VariableDefn::declare(Typechecker* ts) const
	{
		// this does nothing because we don't have the type
		return Ok();
	}

	ErrorOr<TCResult> VariableDefn::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		// if we have neither, it's an error
		if(not this->explicit_type.has_value() && this->initialiser == nullptr)
			return ErrMsg(ts, "variable without explicit type must have an initialiser");

		const Type* var_type = nullptr;
		std::unique_ptr<cst::Expr> init {};

		if(this->explicit_type.has_value())
		{
			var_type = TRY(ts->resolveType(*this->explicit_type));
			if(var_type->isVoid())
				return ErrMsg(ts, "cannot declare variable of type 'void'");

			if(this->initialiser != nullptr)
			{
				init = TRY(this->initialiser->typecheck(ts, /* infer: */ var_type)).take_expr();
				if(not ts->canImplicitlyConvert(init->type(), var_type))
				{
					return ErrMsg(ts, "cannot initialise variable of type '{}' with expression of type '{}'", //
					    var_type, init->type());
				}
			}
		}
		else
		{
			assert(this->initialiser != nullptr);
			init = TRY(this->initialiser->typecheck(ts)).take_expr();
			var_type = init->type();
		}

		auto decl = cst::Declaration(m_location, ts->current(), this->name, var_type, this->is_mutable);
		this->declaration = TRY(ts->current()->declare(std::move(decl)));

		auto defn = std::make_unique<cst::VariableDefn>(m_location, this->declaration, this->is_global, std::move(init),
		    this->is_mutable);

		this->declaration->define(defn.get());
		return TCResult::ofVoid(std::move(defn));
	}
}
