// optional.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> OptionalCheckOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto inside = TRY(this->expr->typecheck(ts, infer));
		if(not inside.type()->isOptional())
			return ErrMsg(ts, "invalid use of '?' on non-optional type '{}'", inside.type());

		return TCResult::ofRValue(Type::makeBool());
	}

	ErrorOr<TCResult2> OptionalCheckOp::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto inside = TRY(this->expr->typecheck2(ts, infer));
		if(not inside.type()->isOptional())
			return ErrMsg(ts, "invalid use of '?' on non-optional type '{}'", inside.type());

		return TCResult2::ofRValue<cst::OptionalCheckOp>(m_location, std::move(inside).take_expr());
	}

	ErrorOr<EvalResult> OptionalCheckOp::evaluate_impl(Evaluator* ev) const
	{
		auto inside = TRY_VALUE(this->expr->evaluate(ev));
		assert(inside.isOptional());

		auto tmp = std::move(inside).takeOptional();
		return EvalResult::ofValue(Value::boolean(tmp.has_value()));
	}





	ErrorOr<TCResult> NullCoalesceOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto ltype = TRY(this->lhs->typecheck(ts)).type();
		auto rtype = TRY(this->rhs->typecheck(ts)).type();

		// this is a little annoying because while we don't want "?int + &int", "?&int and &int" should still work
		if(not ltype->isOptional() && not ltype->isPointer())
			return ErrMsg(ts, "invalid use of '??' with non-pointer, non-optional type '{}' on the left", ltype);

		// the rhs type is either the same as the lhs, or it's the element of the lhs optional/ptr
		auto lelm_type = ltype->isOptional() ? ltype->optionalElement() : ltype->pointerElement();

		bool is_flatmap = (ltype == rtype || ts->canImplicitlyConvert(ltype, rtype));
		bool is_valueor = (lelm_type == rtype || ts->canImplicitlyConvert(lelm_type, rtype));

		if(not(is_flatmap || is_valueor))
			return ErrMsg(ts, "invalid use of '??' with mismatching types '{}' and '{}'", ltype, rtype);

		m_kind = is_flatmap ? Flatmap : ValueOr;

		// return whatever the rhs type is -- be it optional or pointer.
		return TCResult::ofRValue(rtype);
	}


	ErrorOr<TCResult2> NullCoalesceOp::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lres = TRY(this->lhs->typecheck2(ts));
		auto rres = TRY(this->rhs->typecheck2(ts));

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
		return TCResult2::ofRValue<cst::NullCoalesceOp>(m_location, rres.type(),
		    is_flatmap ? cst::NullCoalesceOp::Kind::Flatmap : cst::NullCoalesceOp::Kind::ValueOr,
		    std::move(lres).take_expr(), std::move(rres).take_expr());
	}


	ErrorOr<EvalResult> NullCoalesceOp::evaluate_impl(Evaluator* ev) const
	{
		// short circuit -- don't evaluate rhs eagerly
		auto lval = TRY_VALUE(this->lhs->evaluate(ev));
		auto ltype = lval.type();

		assert(lval.isOptional() || lval.type()->isPointer());
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
