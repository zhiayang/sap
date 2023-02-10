// variable.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"         // for VariableDefn, Expr, Declaration, Var...
#include "interp/type.h"        // for Type
#include "interp/interp.h"      // for Interpreter, DefnTree, StackFrame
#include "interp/eval_result.h" // for EvalResult, TRY_VALUE

namespace sap::interp
{
	ErrorOr<TCResult> VariableDecl::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(infer != nullptr);
		if(infer->isVoid())
			return ErrMsg(ts, "cannot declare variable of type 'void'");

		auto ns = ts->current();
		TRY(ns->declare(this));

		return TCResult::ofLValue(infer, this->is_mutable);
	}

	ErrorOr<EvalResult> VariableDecl::evaluate_impl(Evaluator* ev) const
	{
		// this does nothing
		return EvalResult::ofVoid();
	}




	ErrorOr<TCResult> VariableDefn::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		this->declaration->resolve(this);

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

		return this->declaration->typecheck(ts, the_type.type());
	}

	ErrorOr<EvalResult> VariableDefn::evaluate_impl(Evaluator* ev) const
	{
		if(this->initialiser != nullptr)
		{
			// if this is a global variable that was already initialised, don't re-initialise it
			// the implication is that for layout passes > 1, we don't reset values (we need to persist some state!)
			if(m_is_global && ev->getGlobalValue(this) != nullptr)
				return EvalResult::ofVoid();

			auto value = ev->castValue(TRY_VALUE(this->initialiser->evaluate(ev)), this->get_type());
			if(m_is_global)
				ev->setGlobalValue(this, std::move(value));
			else
				ev->frame().setValue(this, std::move(value));
		}

		return EvalResult::ofVoid();
	}
}
