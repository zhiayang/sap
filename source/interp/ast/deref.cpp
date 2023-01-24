// deref.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> DereferenceOp::typecheck_impl(Typechecker* ts, const Type* infer, bool moving) const
	{
		auto inside = TRY(this->expr->typecheck(ts, infer)).type();
		if(not inside->isOptional() && not inside->isPointer())
			return ErrMsg(ts, "invalid use of '!' on non-pointer, non-optional type '{}'", inside);

		if(inside->isOptional())
			return TCResult::ofRValue(inside->optionalElement());
		else
			return TCResult::ofLValue(inside->pointerElement(), inside->isMutablePointer());
	}

	ErrorOr<EvalResult> DereferenceOp::evaluate_impl(Evaluator* ev) const
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
				return ErrMsg(ev, "dereferencing empty optional");
		}
		else
		{
			auto ptr = inside.getPointer();
			if(ptr == nullptr)
				return ErrMsg(ev, "dereferencing null pointer");

			return EvalResult::ofLValue(const_cast<Value&>(*ptr));
		}
	}






	ErrorOr<TCResult> AddressOfOp::typecheck_impl(Typechecker* ts, const Type* infer, bool moving) const
	{
		auto inside = TRY(this->expr->typecheck(ts, infer));
		if(not inside.isLValue())
			return ErrMsg(ts, "cannot take the address of a non-lvalue");

		if(this->is_mutable && not inside.isMutable())
			return ErrMsg(ts, "cannot create a mutable pointer to an immutable value");

		return TCResult::ofRValue(inside.type()->pointerTo(/* mutable: */ this->is_mutable));
	}

	ErrorOr<EvalResult> AddressOfOp::evaluate_impl(Evaluator* ev) const
	{
		auto expr_res = TRY(this->expr->evaluate(ev));
		assert(expr_res.hasValue());
		assert(expr_res.isLValue());

		auto& inside = expr_res.get();
		auto ty = this->get_type();

		Value ret {};
		if(this->is_mutable)
			ret = Value::mutablePointer(ty->pointerElement(), &inside);
		else
			ret = Value::pointer(ty->pointerElement(), &inside);

		return EvalResult::ofValue(std::move(ret));
	}
}
