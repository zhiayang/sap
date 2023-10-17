// cast.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult2> CastExpr::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto to = TRY(this->target_type->typecheck2(ts)).type();
		auto from = TRY(this->expr->typecheck2(ts, /* infer: */ to));

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

			else
				return ErrMsg(ts, "cannot cast from expression with type '{}' to type '{}'", from.type(), to);
		}

		return TCResult2::ofRValue<cst::CastExpr>(m_location, std::move(from).take_expr(), cast_kind, to);
	}
}
