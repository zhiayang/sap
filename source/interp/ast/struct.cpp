// struct.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<const Type*> StructDecl::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		TRY(cs->current()->declare(this));

		// when declaring a struct, we just make an empty field list.
		// the fields will be set later on.
		return Ok(Type::makeStruct(this->name, {}));
	}

	ErrorOr<const Type*> StructDefn::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		this->declaration->resolved_defn = this;
		auto struct_type = TRY(this->declaration->typecheck(cs))->toStruct();

		std::unordered_set<std::string> seen_names;
		std::vector<StructType::Field> field_types {};

		for(auto& [name, type] : this->fields)
		{
			if(seen_names.contains(name))
				return ErrFmt("duplicate field '{}' in struct '{}'", name, struct_type->name());

			seen_names.insert(name);

			auto field_type = TRY(cs->resolveType(type));
			if(field_type == struct_type)
				return ErrFmt("recursive struct not allowed");
			else if(field_type->isVoid())
				return ErrFmt("field cannot have type 'void'");

			field_types.push_back(StructType::Field {
			    .name = name,
			    .type = field_type,
			});
		}

		const_cast<StructType*>(struct_type)->setFields(std::move(field_types));
		return Ok(struct_type);
	}




	ErrorOr<EvalResult> StructDecl::evaluate(Interpreter* cs) const
	{
		// do nothing
		return Ok(EvalResult::of_void());
	}

	ErrorOr<EvalResult> StructDefn::evaluate(Interpreter* cs) const
	{
		// this also doesn't do anything
		return Ok(EvalResult::of_void());
	}
}
