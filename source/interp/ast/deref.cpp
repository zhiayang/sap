// deref.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> DereferenceOp::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		auto inside = TRY(this->expr->typecheck(ts, infer)).type();
		if(not inside->isOptional() && not inside->isPointer())
			return ErrFmt("invalid use of '!' on non-pointer, non-optional type '{}'", inside);

		if(inside->isOptional())
			return TCResult::ofRValue(inside->optionalElement());
		else
			return TCResult::ofLValue(inside->pointerElement(), inside->isMutablePointer());
	}

	ErrorOr<EvalResult> DereferenceOp::evaluate(Evaluator* ev) const
	{
		auto expr_res = TRY(this->expr->evaluate(ev));
		assert(expr_res.hasValue());

		auto& inside = expr_res.get();
		auto type = inside.type();

		if(type->isOptional())
		{
			auto opt = std::move(inside).takeOptional();
			if(opt.has_value())
				return EvalResult::ofValue(std::move(*opt));
			else
				return ErrFmt("dereferencing empty optional");
		}
		else
		{
			auto ptr = inside.getPointer();
			if(ptr == nullptr)
				return ErrFmt("dereferencing null pointer");

			return EvalResult::ofLValue(const_cast<Value&>(*ptr));
		}
	}
}
