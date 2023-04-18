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
		return TCResult::ofRValue(Type::makeStruct(this->name, {}));
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







}
