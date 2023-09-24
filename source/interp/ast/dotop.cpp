// dotop.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> DotOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
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

	ErrorOr<TCResult2> DotOp::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lhs_res = TRY(this->lhs->typecheck2(ts));
		auto ltype = lhs_res.type();

		const StructType* struct_type = nullptr;
		if(this->is_optional)
		{
			if(not ltype->isOptional() && not ltype->isPointer())
				return ErrMsg(ts, "invalid use of '?.' operator on a non-pointer, non-optional type '{}'", ltype);

			auto lelm_type = ltype->isPointer() ? ltype->pointerElement() : ltype->optionalElement();
			if(not lelm_type->isStruct())
				return ErrMsg(ts, "invalid use of '?.' operator on a non-struct type '{}'", lelm_type);

			struct_type = lelm_type->toStruct();
		}
		else
		{
			if(not ltype->isStruct())
				return ErrMsg(ts, "invalid use of '.' operator on a non-struct type '{}'", ltype);

			struct_type = ltype->toStruct();
		}

		assert(struct_type != nullptr);
		if(not struct_type->hasFieldNamed(this->rhs))
			return ErrMsg(ts, "type '{}' has no field named '{}'", ltype, this->rhs);

		auto field_type = struct_type->getFieldNamed(this->rhs);
		const Type* result_type = [&]() {
			if(this->is_optional)
			{
				return ltype->isOptional() //
				         ? (const Type*) field_type->optionalOf()
				         : (const Type*) field_type->pointerTo(ltype->isMutablePointer());
			}
			else
			{
				return field_type;
			}
		}();

		bool is_lvalue = lhs_res.isLValue();
		bool is_mutable = lhs_res.isMutable();

		auto dot_op = std::make_unique<cst::DotOp>(m_location, result_type, this->is_optional, struct_type,
		    std::move(lhs_res).take_expr(), this->rhs);

		if(this->is_optional)
		{
			// if we're optional, make an rvalue always.
			// this means we can't do a?.b = ... , but that's fine probably.
			return TCResult2::ofRValue(std::move(dot_op));
		}
		else
		{
			// otherwise, we copy the lvalue-ness and mutability of the lhs.
			if(is_lvalue)
				return TCResult2::ofLValue(std::move(dot_op), is_mutable);
			else
				return TCResult2::ofRValue(std::move(dot_op));
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
			                   || (lhs_value.type()->isPointer() && lhs_value.getPointer() != nullptr);

			if(lhs_value.type()->isPointer())
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
