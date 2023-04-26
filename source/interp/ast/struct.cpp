// struct.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"
#include "interp/overload_resolution.h"

namespace sap::interp
{
	ErrorOr<TCResult> StructDecl::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		TRY(ts->current()->declare(this));

		// when declaring a struct, we just make an empty field list.
		// the fields will be set later on.
		return TCResult::ofRValue(Type::makeStruct(m_declared_tree->scopeName(this->name), {}));
	}

	ErrorOr<TCResult> StructDefn::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		this->declaration->resolve(this);
		auto struct_type = TRY(this->declaration->typecheck(ts)).type()->toStruct();

		util::hashset<std::string> seen_names;
		std::vector<StructType::Field> field_types {};

		for(auto& [name, type, init_value] : m_fields)
		{
			if(seen_names.contains(name))
				return ErrMsg(ts, "duplicate field '{}' in struct '{}'", name, struct_type->name());

			seen_names.insert(name);

			auto field_type = TRY(ts->resolveType(type));
			if(field_type == struct_type)
				return ErrMsg(ts, "recursive struct not allowed");
			else if(field_type->isVoid())
				return ErrMsg(ts, "field cannot have type 'void'");

			if(init_value != nullptr)
			{
				auto initialiser_type = TRY(init_value->typecheck(ts)).type();
				if(not ts->canImplicitlyConvert(initialiser_type, field_type))
					return ErrMsg(ts, "cannot initialise field of type '{}' with value of type '{}'", field_type,
					    initialiser_type);
			}

			field_types.push_back(StructType::Field {
			    .name = name,
			    .type = field_type,
			});
		}

		const_cast<StructType*>(struct_type)->setFields(std::move(field_types));
		TRY(ts->addTypeDefinition(struct_type, this));
		return TCResult::ofRValue(struct_type);
	}




	ErrorOr<EvalResult> StructDecl::evaluate_impl(Evaluator* ev) const
	{
		// do nothing
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> StructDefn::evaluate_impl(Evaluator* ev) const
	{
		// this also doesn't do anything
		return EvalResult::ofVoid();
	}






	ErrorOr<TCResult> StructUpdateOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lhs = TRY(this->structure->typecheck(ts, infer));
		if(not lhs.type()->isStruct())
			return ErrMsg(ts, "left-hand side of '//' operator must be a struct; got '{}'", lhs.type());

		auto struct_type = lhs.type()->toStruct();
		auto _ = ts->pushStructFieldContext(struct_type);

		for(auto& [name, expr] : this->updates)
		{
			if(not struct_type->hasFieldNamed(name))
				return ErrMsg(expr->loc(), "struct '{}' has no field named '{}'", struct_type->name().name, name);

			auto field_type = struct_type->getFieldNamed(name);
			auto expr_type = TRY(expr->typecheck(ts, field_type)).type();

			if(not ts->canImplicitlyConvert(expr_type, field_type))
			{
				return ErrMsg(expr->loc(), "cannot update struct field '{}' with type '{}' using a value of type '{}'",
				    name, field_type, expr_type);
			}
		}

		return TCResult::ofRValue(struct_type);
	}

	ErrorOr<EvalResult> StructUpdateOp::evaluate_impl(Evaluator* ev) const
	{
		auto struct_val = TRY_VALUE(this->structure->evaluate(ev));
		auto struct_type = struct_val.type()->toStruct();

		auto _ctx = ev->pushStructFieldContext(&struct_val);

		util::hashmap<size_t, Value> new_values {};
		for(auto& [name, expr] : this->updates)
		{
			auto field_idx = struct_type->getFieldIndex(name);
			auto field_type = struct_type->getFieldAtIndex(field_idx);

			auto value = TRY_VALUE(expr->evaluate(ev));
			value = ev->castValue(std::move(value), field_type);

			new_values.emplace(field_idx, std::move(value));
		}

		// update them all at once.
		for(auto& [idx, val] : new_values)
			struct_val.getStructField(idx) = std::move(val);

		return EvalResult::ofValue(std::move(struct_val));
	}
}
