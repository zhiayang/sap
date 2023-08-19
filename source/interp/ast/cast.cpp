// cast.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> CastExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto to = TRY(this->target_type->typecheck(ts)).type();
		auto from = TRY(this->expr->typecheck(ts, /* infer: */ to)).type();

		// TODO: warn for redundant casts (ie. where we already have implicit conversions,
		// or where to == from)
		if(ts->canImplicitlyConvert(from, to))
		{
			m_cast_kind = CastKind::Implicit;
		}
		else
		{
			if(from->isFloating() && to->isInteger())
				m_cast_kind = CastKind::FloatToInteger;

			else if(from->isChar() && to->isInteger())
				m_cast_kind = CastKind::CharToInteger;

			else if(from->isInteger() && to->isChar())
				m_cast_kind = CastKind::IntegerToChar;

			else
				return ErrMsg(ts, "cannot cast from expression with type '{}' to type '{}'", from, to);
		}

		return TCResult::ofRValue(to);
	}

	ErrorOr<EvalResult> CastExpr::evaluate_impl(Evaluator* ev) const
	{
		auto val = TRY_VALUE(this->expr->evaluate(ev));

		Value result {};
		switch(m_cast_kind)
		{
			using enum CastKind;

			case None: return ErrMsg(ev, "invalid cast!");

			case Implicit: //
				result = ev->castValue(std::move(val), this->get_type());
				break;

			case FloatToInteger: //
				result = Value::integer(static_cast<int64_t>(val.getFloating()));
				break;

			case CharToInteger: //
				result = Value::integer(static_cast<int64_t>(val.getChar()));
				break;

			case IntegerToChar: //
				result = Value::character(static_cast<char32_t>(val.getInteger()));
				break;
		}

		return EvalResult::ofValue(std::move(result));
	}
}
