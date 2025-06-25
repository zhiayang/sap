// cast.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> CastExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto to = TRY(this->target_type->typecheck(ts)).type();
		auto from = TRY(this->expr->typecheck(ts, /* infer: */ to));

		cst::CastExpr::CastKind cast_kind;

		// TODO: warn for redundant casts (ie. where we already have implicit conversions,
		// or where to == from)
		if(ts->canImplicitlyConvert(from.type(), to))
		{
			cast_kind = cst::CastExpr::CastKind::Implicit;
		}
		else
		{
			if(from.type()->isFloating() && to->isInteger())
				cast_kind = cst::CastExpr::CastKind::FloatToInteger;
			else if(from.type()->isChar() && to->isInteger())
				cast_kind = cst::CastExpr::CastKind::CharToInteger;
			else if(from.type()->isInteger() && to->isChar())
				cast_kind = cst::CastExpr::CastKind::IntegerToChar;
			else if(from.type()->isPointer() && to->isPointer())
				cast_kind = cst::CastExpr::CastKind::Pointer;
			else
				return ErrMsg(ts, "cannot cast from expression with type '{}' to type '{}'", from.type(), to);
		}

		return TCResult::ofRValue<cst::CastExpr>(m_location, std::move(from).take_expr(), cast_kind, to);
	}

	ErrorOr<TCResult> ImplicitUnionVariantCastExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue)
	    const
	{
		// the expression must have a type already
		auto from = TRY(this->expr->typecheck(ts, /* infer: */ nullptr, /* keep_lvalue: */ keep_lvalue));
		if(not from.type()->isUnion())
			return ErrMsg(ts, "cannot cast an expression with non-union type '{}' to a union variant", from.type());

		const auto union_type = from.type()->toUnion();
		if(not union_type->hasCaseNamed(this->variant_name))
			return ErrMsg(ts, "union '{}' has no variant named '{}'", union_type->name(), this->variant_name);

		const auto to = union_type->getCaseNamed(this->variant_name);

		auto result = std::make_unique<cst::UnionVariantCastExpr>(m_location, std::move(from).take_expr(), to,
		    union_type, union_type->getCaseIndex(this->variant_name));

		if(keep_lvalue && from.isLValue())
			return TCResult::ofLValue(std::move(result), from.isMutable());
		else
			return TCResult::ofRValue(std::move(result));
	}
}
