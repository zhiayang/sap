// dotop.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> DotOp::evaluate_impl(Evaluator* ev) const
	{
		assert(this->struct_type->hasFieldNamed(this->field_name));
		auto field_idx = this->struct_type->getFieldIndex(this->field_name);
		auto field_type = this->struct_type->getFieldAtIndex(field_idx);

		auto lhs_res = TRY(this->expr->evaluate(ev));
		if(not lhs_res.hasValue())
			sap::internal_error("unexpected void value");

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
