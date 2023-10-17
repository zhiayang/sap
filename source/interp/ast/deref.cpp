// deref.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
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

	ErrorOr<TCResult2> AddressOfOp::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto inside = TRY(this->expr->typecheck2(ts, infer, /* keep_lvalue: */ true));
		if(not inside.isLValue())
			return ErrMsg(ts, "cannot take the address of a non-lvalue");

		if(this->is_mutable && not inside.isMutable())
			return ErrMsg(ts, "cannot create a mutable pointer to an immutable value");

		auto out_type = inside.type()->pointerTo(/* mutable: */ this->is_mutable);
		return TCResult2::ofRValue<cst::AddressOfOp>(m_location, out_type, this->is_mutable, std::move(inside).take_expr());
	}
}
