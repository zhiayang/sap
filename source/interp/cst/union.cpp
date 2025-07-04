// union.cpp
// Copyright (c) 2023, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"
#include "interp/overload_resolution.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> UnionDefn::evaluate_impl(Evaluator* ev) const
	{
		// do nothing
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> UnionExpr::evaluate_impl(Evaluator* ev) const
	{
		auto union_type = this->type()->toUnion();

		// make the case value as a struct.
		auto case_type = union_type->getCaseAtIndex(this->case_index);
		std::vector<Value> field_values {};
		for(size_t i = 0; i < this->values.size(); i++)
		{
			auto& f = this->values[i];
			auto ft = case_type->getFieldTypes()[i];

			auto v = TRY_VALUE(f->evaluate(ev));
			field_values.push_back(ev->castValue(std::move(v), ft));
		}

		auto case_value = Value::structure(case_type, std::move(field_values));
		return EvalResult::ofValue(Value::unionVariant(union_type, this->case_index, std::move(case_value)));
	}
}
