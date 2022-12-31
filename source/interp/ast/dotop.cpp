// dotop.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> DotOp::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		auto lhs_res = TRY(this->lhs->typecheck(cs));

		if(not lhs_res.type()->isStruct())
			return ErrFmt("invalid use of '.' operator on a non-struct type '{}'", lhs_res.type());

		auto struct_type = lhs_res.type()->toStruct();
		if(not struct_type->hasFieldNamed(this->rhs))
			return ErrFmt("type '{}' has no field named '{}'", lhs_res.type(), this->rhs);

		// basically, we copy the lvalue-ness and mutability of the lhs.
		return Ok(lhs_res.replacingType(struct_type->getFieldNamed(this->rhs)));
	}

	ErrorOr<EvalResult> DotOp::evaluate(Interpreter* cs) const
	{
		auto lhs_type = this->lhs->get_type();
		assert(lhs_type->isStruct());

		auto lhs_res = TRY(this->lhs->evaluate(cs));
		if(not lhs_res.hasValue())
			return ErrFmt("unexpected void value");

		auto& lhs_value = lhs_res.get();
		if(not lhs_value.isStruct() || lhs_value.type() != lhs_type)
			return ErrFmt("unexpected type '{}' in dotop", lhs_value.type());

		auto struct_type = lhs_type->toStruct();
		assert(struct_type->hasFieldNamed(this->rhs));

		auto field_idx = struct_type->getFieldIndex(this->rhs);

		if(lhs_res.isLValue())
			return EvalResult::ofLValue(lhs_value.getStructField(field_idx));
		else
			return EvalResult::ofValue(lhs_value.getStructField(field_idx).clone());
	}
}
