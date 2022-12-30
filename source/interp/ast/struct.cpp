// struct.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/misc.h"
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



	ErrorOr<const Type*> StructLit::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		const StructType* struct_type = nullptr;
		if(struct_name.name.empty())
		{
			if(infer == nullptr)
				return ErrFmt("cannot infer type of struct literal");
			else if(not infer->isStruct())
				return ErrFmt("inferred non-struct type '{}' for struct literal", infer);

			struct_type = infer->toStruct();
		}
		else
		{
			auto t = TRY(cs->resolveType(frontend::PType::named(this->struct_name)));
			if(not t->isStruct())
				return ErrFmt("invalid non-struct type '{}' for struct literal", t);

			struct_type = t->toStruct();
		}

		// make sure the struct has all the things
		std::vector<std::tuple<std::string, const Type*, const Expr*>> fields {};

		auto& struct_fields = struct_type->getFields();
		for(size_t i = 0; i < struct_fields.size(); i++)
		{
			fields.push_back({
			    struct_fields[i].name,
			    struct_fields[i].type,
			    nullptr,
			});
		}

		auto ordered = TRY(arrange_arguments<const Type*>(cs, fields, this->field_inits, //
		    "struct", "field", "field", [cs](auto& arg) {
			    return arg.value->typecheck(cs);
		    }));

		TRY(get_calling_cost(cs, fields, ordered, "struct", "field", "field"));
		return Ok(struct_type);
	}

	ErrorOr<EvalResult> StructLit::evaluate(Interpreter* cs) const
	{
		// this also doesn't do anything
		return Ok(EvalResult::of_void());
	}
}
