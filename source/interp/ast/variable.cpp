// variable.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"         // for VariableDefn, Expr, Declaration, Var...
#include "interp/type.h"        // for Type
#include "interp/interp.h"      // for Interpreter, DefnTree, StackFrame
#include "interp/eval_result.h" // for EvalResult, TRY_VALUE

namespace sap::interp
{
	ErrorOr<TCResult> VariableDecl::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		assert(infer != nullptr);
		if(infer->isVoid())
			return ErrFmt("cannot declare variable of type 'void'");

		auto ns = cs->current();
		TRY(ns->declare(this));

		return TCResult::ofLValue(infer, this->is_mutable);
	}

	ErrorOr<EvalResult> VariableDecl::evaluate(Interpreter* cs) const
	{
		// this does nothing
		return EvalResult::ofVoid();
	}




	ErrorOr<TCResult> VariableDefn::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		this->declaration->resolved_defn = this;

		// if we have neither, it's an error
		if(not this->explicit_type.has_value() && this->initialiser == nullptr)
			return ErrFmt("variable without explicit type must have an initialiser");

		auto the_type = TRY([&]() -> ErrorOr<TCResult> {
			if(this->explicit_type.has_value())
			{
				auto resolved_type = TRY(cs->resolveType(*this->explicit_type));

				if(resolved_type->isVoid())
					return ErrFmt("cannot declare variable of type 'void'");

				if(this->initialiser != nullptr)
				{
					auto initialiser_type = TRY(this->initialiser->typecheck(cs, /* infer: */ resolved_type)).type();
					if(not cs->canImplicitlyConvert(initialiser_type, resolved_type))
					{
						return ErrFmt("cannot initialise variable of type '{}' with expression of type '{}'", //
						    resolved_type, initialiser_type);
					}
				}

				return TCResult::ofRValue(resolved_type);
			}
			else
			{
				assert(this->initialiser != nullptr);
				return this->initialiser->typecheck(cs);
			}
		}());

		return this->declaration->typecheck(cs, the_type.type());
	}

	ErrorOr<EvalResult> VariableDefn::evaluate(Interpreter* cs) const
	{
		if(this->initialiser != nullptr)
		{
			auto value = cs->castValue(TRY_VALUE(this->initialiser->evaluate(cs)), this->get_type());
			cs->frame().setValue(this, std::move(value));
		}

		return EvalResult::ofVoid();
	}
}
