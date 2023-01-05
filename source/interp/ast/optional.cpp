// optional.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> OptionalCheckOp::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		auto inside = TRY(this->expr->typecheck(ts, infer));
		if(not inside.type()->isOptional())
			return ErrFmt("invalid use of '?' on non-optional type '{}'", inside.type());

		return TCResult::ofRValue(Type::makeBool());
	}

	ErrorOr<EvalResult> OptionalCheckOp::evaluate(Evaluator* ev) const
	{
		auto inside = TRY_VALUE(this->expr->evaluate(ev));
		assert(inside.isOptional());

		auto tmp = std::move(inside).takeOptional();
		return EvalResult::ofValue(Value::boolean(tmp.has_value()));
	}





	ErrorOr<TCResult> NullCoalesceOp::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		auto ltype = TRY(this->lhs->typecheck(ts)).type();
		auto rtype = TRY(this->rhs->typecheck(ts)).type();

		// this is a little annoying because while we don't want "?int + &int", "?&int and &int" should still work
		if(not ltype->isOptional() && not ltype->isPointer())
			return ErrFmt("invalid use of '??' with non-pointer, non-optional type '{}' on the left", ltype);

		// the rhs type is either the same as the lhs, or it's the element of the lhs optional/ptr
		auto lelm_type = ltype->isOptional() ? ltype->optionalElement() : ltype->pointerElement();

		bool is_flatmap = (ltype == rtype || ts->canImplicitlyConvert(ltype, rtype));
		bool is_valueor = (lelm_type == rtype || ts->canImplicitlyConvert(lelm_type, rtype));

		if(not(is_flatmap || is_valueor))
			return ErrFmt("invalid use of '??' with mismatching types '{}' and '{}'", ltype, rtype);

		m_kind = is_flatmap ? Flatmap : ValueOr;

		// return whatever the rhs type is -- be it optional or pointer.
		return TCResult::ofRValue(rtype);
	}

	ErrorOr<EvalResult> NullCoalesceOp::evaluate(Evaluator* ev) const
	{
		// short circuit -- don't evaluate rhs eagerly
		auto lval = TRY_VALUE(this->lhs->evaluate(ev));
		auto ltype = lval.type();

		assert(lval.isOptional() || lval.isPointer());
		bool left_has_value = (lval.isOptional() && lval.haveOptionalValue())
		                   || (lval.isPointer() && lval.getPointer() != nullptr);

		if(left_has_value)
		{
			auto result_type = this->get_type();

			// if it's the same, then just return it. this means that both the left and right
			// sides were either pointers or optionals.
			if(m_kind == Flatmap)
				return EvalResult::ofValue(ev->castValue(std::move(lval), result_type));

			// otherwise, the right type is the "element" of the left type
			if(ltype->isOptional())
			{
				auto ret = ev->castValue(std::move(lval).takeOptional().value(), result_type);
				return EvalResult::ofValue(std::move(ret));
			}
			else
			{
				auto ret = ev->castValue(lval.getPointer()->clone(), result_type);
				return EvalResult::ofValue(std::move(ret));
			}
		}
		else
		{
			// just evaluate the right-hand side.
			return EvalResult::ofValue(TRY_VALUE(this->rhs->evaluate(ev)));
		}
	}
}
