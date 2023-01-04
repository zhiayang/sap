// dotop.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	static const StructType* get_struct_type(const Type* t)
	{
		if(t->isStruct())
			return t->toStruct();
		else if(t->isPointer() && t->toPointer()->elementType()->isStruct())
			return t->toPointer()->elementType()->toStruct();
		else
			return nullptr;
	}

	ErrorOr<TCResult> DotOp::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		auto lhs_res = TRY(this->lhs->typecheck(ts));

		auto struct_type = get_struct_type(lhs_res.type());
		if(struct_type == nullptr)
			return ErrFmt("invalid use of '.' operator on a non-struct type '{}'", lhs_res.type());

		if(not struct_type->hasFieldNamed(this->rhs))
			return ErrFmt("type '{}' has no field named '{}'", lhs_res.type(), this->rhs);

		// basically, we copy the lvalue-ness and mutability of the lhs.
		return Ok(lhs_res.replacingType(struct_type->getFieldNamed(this->rhs)));
	}

	ErrorOr<EvalResult> DotOp::evaluate(Evaluator* ev) const
	{
		auto lhs_res = TRY(this->lhs->evaluate(ev));
		if(not lhs_res.hasValue())
			return ErrFmt("unexpected void value");

		auto& lhs_value = lhs_res.get();
		if(lhs_value.isPointer() && lhs_value.getPointer() == nullptr)
			return ErrFmt("null pointer dereference");

		auto struct_type = get_struct_type(this->lhs->get_type());
		if(auto value_type = get_struct_type(lhs_value.type()); value_type == nullptr || value_type != struct_type)
			return ErrFmt("unexpected type '{}' in dotop", lhs_value.type());

		assert(struct_type->hasFieldNamed(this->rhs));

		auto field_idx = struct_type->getFieldIndex(this->rhs);

		if(lhs_value.isPointer())
		{
			if(lhs_value.type()->isMutablePointer())
				return EvalResult::ofLValue(lhs_value.getMutablePointer()->getStructField(field_idx));
			else
				return EvalResult::ofValue(lhs_value.getPointer()->getStructField(field_idx).clone());
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
