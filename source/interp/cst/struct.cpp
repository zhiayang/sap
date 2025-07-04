// struct.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"
#include "interp/overload_resolution.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> StructDefn::evaluate_impl(Evaluator* ev) const
	{
		// do nothing
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> StructLit::evaluate_impl(Evaluator* ev) const
	{
		assert(m_type->isStruct());
		auto struct_type = m_type->toStruct();

		std::vector<Value> field_values {};
		for(size_t i = 0; i < this->field_inits.size(); i++)
		{
			auto& f = this->field_inits[i];
			auto ft = struct_type->getFieldTypes()[i];

			auto v = TRY_VALUE(f->evaluate(ev));
			field_values.push_back(ev->castValue(std::move(v), ft));
		}

		return EvalResult::ofValue(Value::structure(struct_type, std::move(field_values)));
	}

	ErrorOr<EvalResult> StructUpdateOp::evaluate_impl(Evaluator* ev) const
	{
		Value* struct_ptr = nullptr;
		Value _struct_val;
		if(this->is_assignment)
		{
			struct_ptr = TRY(this->structure->evaluate(ev)).lValuePointer();
		}
		else
		{
			_struct_val = TRY_VALUE(this->structure->evaluate(ev));
			struct_ptr = &_struct_val;
		}

		auto struct_type = struct_ptr->type()->toStruct();
		auto _ctx = ev->pushStructFieldContext(struct_ptr);

		util::hashmap<size_t, Value> new_values {};
		for(auto& [name, expr] : this->updates)
		{
			auto field_idx = struct_type->getFieldIndex(name);
			auto field_type = struct_type->getFieldAtIndex(field_idx);

			if(this->is_optional && field_type->isOptional())
			{
				if(struct_ptr->getStructField(field_idx).haveOptionalValue())
					continue;
			}

			auto value = TRY_VALUE(expr->evaluate(ev));
			value = ev->castValue(std::move(value), field_type);

			new_values.emplace(field_idx, std::move(value));
		}

		// update them all at once.
		for(auto& [idx, val] : new_values)
			struct_ptr->getStructField(idx) = std::move(val);

		if(this->is_assignment)
			return EvalResult::ofVoid();
		else
			return EvalResult::ofValue(std::move(_struct_val));
	}


	ErrorOr<EvalResult> StructContextSelf::evaluate_impl(Evaluator* ev) const
	{
		auto& ctx = ev->getStructFieldContext();
		return EvalResult::ofLValue(ctx);
	}
}
