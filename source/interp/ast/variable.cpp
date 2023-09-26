// variable.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"         // for VariableDefn, Expr, Declaration, Var...
#include "interp/type.h"        // for Type
#include "interp/interp.h"      // for Interpreter, DefnTree, StackFrame
#include "interp/eval_result.h" // for EvalResult, TRY_VALUE

namespace sap::interp::ast
{
	ErrorOr<void> VariableDefn::declare(Typechecker* ts) const
	{
		// if we have neither, it's an error
		if(not this->explicit_type.has_value() && this->initialiser == nullptr)
			return ErrMsg(ts, "variable without explicit type must have an initialiser");

		auto the_type = TRY([&]() -> ErrorOr<TCResult> {
			if(this->explicit_type.has_value())
			{
				auto resolved_type = TRY(ts->resolveType(*this->explicit_type));

				if(resolved_type->isVoid())
					return ErrMsg(ts, "cannot declare variable of type 'void'");

				if(this->initialiser != nullptr)
				{
					auto initialiser_type = TRY(this->initialiser->typecheck(ts, /* infer: */ resolved_type)).type();
					if(not ts->canImplicitlyConvert(initialiser_type, resolved_type))
					{
						return ErrMsg(ts, "cannot initialise variable of type '{}' with expression of type '{}'", //
						    resolved_type, initialiser_type);
					}
				}

				return TCResult::ofRValue(resolved_type);
			}
			else
			{
				assert(this->initialiser != nullptr);
				return this->initialiser->typecheck(ts);
			}
		}());

		auto decl = cst::Declaration(m_location, ts->current(), this->name, the_type.type(), this->is_mutable);
		this->declaration = TRY(ts->current()->declare(std::move(decl)));

		return Ok();
	}

	ErrorOr<TCResult> VariableDefn::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofVoid();
	}

	ErrorOr<TCResult2> VariableDefn::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(this->declaration != nullptr);

		std::unique_ptr<cst::Expr> init {};
		if(this->initialiser)
			init = TRY(this->initialiser->typecheck2(ts)).take_expr();

		auto defn = std::make_unique<cst::VariableDefn>(m_location, this->declaration, this->is_global, std::move(init),
		    this->is_mutable);

		this->declaration->define(defn.get());
		return TCResult2::ofVoid(std::move(defn));
	}

	ErrorOr<EvalResult> VariableDefn::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofVoid();
	}
}
