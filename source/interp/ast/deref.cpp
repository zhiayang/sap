// deref.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> DereferenceOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto inside = TRY(this->expr->typecheck(ts, infer)).type();
		if(not inside->isOptional() && not inside->isPointer())
			return ErrMsg(ts, "invalid use of '!' on non-pointer, non-optional type '{}'", inside);

		if(inside->isOptional())
			return TCResult::ofRValue(inside->optionalElement());
		else
			return TCResult::ofLValue(inside->pointerElement(), inside->isMutablePointer());
	}

	ErrorOr<TCResult2> DereferenceOp::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto inside = TRY(this->expr->typecheck2(ts, infer));
		auto inside_type = inside.type();

		if(not inside_type->isOptional() && not inside_type->isPointer())
			return ErrMsg(ts, "invalid use of '!' on non-pointer, non-optional type '{}'", inside_type);

		if(inside_type->isOptional())
		{
			return TCResult2::ofRValue<cst::DereferenceOp>(m_location, inside_type->optionalElement(),
			    std::move(inside).take_expr());
		}
		else if(inside_type->isMutablePointer())
		{
			return TCResult2::ofMutableLValue<cst::DereferenceOp>(m_location, inside_type->pointerElement(),
			    std::move(inside).take_expr());
		}
		else
		{
			return TCResult2::ofImmutableLValue<cst::DereferenceOp>(m_location, inside_type->pointerElement(),
			    std::move(inside).take_expr());
		}
	}


	ErrorOr<EvalResult> DereferenceOp::evaluate_impl(Evaluator* ev) const
	{
		auto expr_res = TRY(this->expr->evaluate(ev));
		assert(expr_res.hasValue());

		auto& inside = expr_res.get();
		auto type = inside.type();

		if(type->isOptional())
		{
			auto opt = inside.getOptional();
			if(opt.has_value())
			{
				if(expr_res.isLValue())
					return EvalResult::ofValue((*opt)->clone());
				else
					return EvalResult::ofValue(std::move(**opt));
			}
			else
			{
				return ErrMsg(ev, "dereferencing empty optional");
			}
		}
		else
		{
			auto ptr = inside.getPointer();
			if(ptr == nullptr)
				return ErrMsg(ev, "dereferencing null pointer");

			return EvalResult::ofLValue(const_cast<Value&>(*ptr));
		}
	}






	ErrorOr<TCResult> AddressOfOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto inside = TRY(this->expr->typecheck(ts, infer, /* keep_lvalue: */ true));
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
		if(auto inside_ty = inside.type();
		    inside_ty->isTreeBlockObj() || inside_ty->isTreeInlineObj() || inside_ty->isLayoutObject())
		{
			return EvalResult::ofVoid();
		}
		else
		{
			auto ty = this->get_type();

			Value ret {};
			if(this->is_mutable)
				ret = Value::mutablePointer(ty->pointerElement(), &inside);
			else
				ret = Value::pointer(ty->pointerElement(), &inside);

			return EvalResult::ofValue(std::move(ret));
		}
	}
}
