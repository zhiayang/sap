// dotop.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<const Type*> DotOp::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		auto lhs_type = TRY(this->lhs->typecheck(cs));
		if(not lhs_type->isStruct())
			return ErrFmt("invalid use of '.' operator on a non-struct type '{}'", lhs_type);

		auto struct_type = lhs_type->toStruct();
		if(not struct_type->hasFieldNamed(this->rhs))
			return ErrFmt("type '{}' has no field named '{}'", lhs_type, this->rhs);

		return Ok(struct_type->getFieldNamed(this->rhs));
	}

	ErrorOr<EvalResult> DotOp::evaluate(Interpreter* cs) const
	{
		auto lhs_type = this->lhs->get_type();
		assert(lhs_type->isStruct());

		auto lhs_value = TRY_VALUE(this->lhs->evaluate(cs));
		if(not lhs_value.isStruct() || lhs_value.type() != lhs_type)
			return ErrFmt("unexpected type '{}' in dotop", lhs_value.type());

		auto struct_type = lhs_type->toStruct();
		assert(struct_type->hasFieldNamed(this->rhs));

		auto field_idx = struct_type->getFieldIndex(this->rhs);
		return Ok(EvalResult::of_value(lhs_value.getStructField(field_idx).clone()));
	}
}
