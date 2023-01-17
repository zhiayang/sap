// dotop.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> DotOp::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		auto lhs_res = TRY(this->lhs->typecheck(ts));
		auto ltype = lhs_res.type();

		if(this->is_optional)
		{
			if(not ltype->isOptional() && not ltype->isPointer())
				return ErrMsg(ts, "invalid use of '?.' operator on a non-pointer, non-optional type '{}'", ltype);

			auto lelm_type = ltype->isPointer() ? ltype->pointerElement() : ltype->optionalElement();
			if(not lelm_type->isStruct())
				return ErrMsg(ts, "invalid use of '?.' operator on a non-struct type '{}'", lelm_type);

			m_struct_type = lelm_type->toStruct();
		}
		else
		{
			if(not ltype->isStruct())
				return ErrMsg(ts, "invalid use of '.' operator on a non-struct type '{}'", ltype);

			m_struct_type = ltype->toStruct();
		}

		assert(m_struct_type != nullptr);
		if(not m_struct_type->hasFieldNamed(this->rhs))
			return ErrMsg(ts, "type '{}' has no field named '{}'", ltype, this->rhs);

		auto field_type = m_struct_type->getFieldNamed(this->rhs);
		if(this->is_optional)
		{
			// if we're optional, make an rvalue always.
			// this means we can't do a?.b = ... , but that's fine probably.
			return TCResult::ofRValue(
			    ltype->isOptional() //
			        ? (const Type*) field_type->optionalOf()
			        : (const Type*) field_type->pointerTo(ltype->isMutablePointer()));
		}
		else
		{
			// otherwise, we copy the lvalue-ness and mutability of the lhs.
			return Ok(lhs_res.replacingType(field_type));
		}
	}

	ErrorOr<EvalResult> DotOp::evaluate_impl(Evaluator* ev) const
	{
		assert(m_struct_type->hasFieldNamed(this->rhs));
		auto field_idx = m_struct_type->getFieldIndex(this->rhs);
		auto field_type = m_struct_type->getFieldAtIndex(field_idx);

		auto lhs_res = TRY(this->lhs->evaluate(ev));
		if(not lhs_res.hasValue())
			return ErrMsg(ev, "unexpected void value");

		auto& lhs_value = lhs_res.get();
		auto ltype = lhs_value.type();
		if(this->is_optional)
		{
			bool left_has_value = (lhs_value.isOptional() && lhs_value.haveOptionalValue())
			                   || (lhs_value.isPointer() && lhs_value.getPointer() != nullptr);

			if(lhs_value.isPointer())
			{
				Value* result = nullptr;
				if(left_has_value)
					result = &lhs_value.getStructField(field_idx);

				if(ltype->isMutablePointer())
					return EvalResult::ofLValue(*result);
				else
					return EvalResult::ofValue(Value::pointer(field_type, result));
			}
			else
			{
				if(left_has_value)
				{
					auto fields = std::move(lhs_value).takeOptional().value().takeStructFields();
					return EvalResult::ofValue(Value::optional(field_type, std::move(fields[field_idx])));
				}
				else
				{
					return EvalResult::ofValue(Value::optional(field_type, std::nullopt));
				}
			}
		}
		else
		{
			if(lhs_res.isLValue())
				return EvalResult::ofLValue(lhs_value.getStructField(field_idx));
			else
				return EvalResult::ofValue(lhs_value.getStructField(field_idx).clone());
		}
	}
}
