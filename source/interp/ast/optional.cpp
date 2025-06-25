// optional.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> NullCoalesceOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lres = TRY(this->lhs->typecheck(ts));
		auto rres = TRY(this->rhs->typecheck(ts));

		// this is a little annoying because while we don't want "?int + &int", "?&int and &int" should still work
		if(not lres.type()->isOptional() && not lres.type()->isPointer())
			return ErrMsg(ts, "invalid use of '??' with non-pointer, non-optional type '{}' on the left", lres.type());

		// the rhs type is either the same as the lhs, or it's the element of the lhs optional/ptr
		auto lelm_type = lres.type()->isOptional() ? lres.type()->optionalElement() : lres.type()->pointerElement();

		bool is_flatmap = (lres.type() == rres.type() || ts->canImplicitlyConvert(lres.type(), rres.type()));
		bool is_valueor = (lelm_type == rres.type() || ts->canImplicitlyConvert(lelm_type, rres.type()));

		if(not(is_flatmap || is_valueor))
			return ErrMsg(ts, "invalid use of '??' with mismatching types '{}' and '{}'", lres.type(), rres.type());

		// return whatever the rhs type is -- be it optional or pointer.
		return TCResult::ofRValue<cst::NullCoalesceOp>(m_location, rres.type(),
		    is_flatmap ? cst::NullCoalesceOp::Kind::Flatmap : cst::NullCoalesceOp::Kind::ValueOr,
		    std::move(lres).take_expr(), std::move(rres).take_expr());
	}
}
