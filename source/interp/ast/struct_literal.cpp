// struct_literal.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"
#include "interp/overload_resolution.h"

namespace sap::interp::ast
{
	static ExpectedParams get_field_things(const cst::StructDefn* struct_defn, const StructType* struct_type)
	{
		ExpectedParams fields {};

		auto& struct_fields = struct_type->getFields();
		for(size_t i = 0; i < struct_fields.size(); i++)
		{
			fields.push_back({
			    .name = struct_fields[i].name,
			    .type = struct_fields[i].type,
			    .default_value = struct_defn->fields[i].initialiser.get(),
			});
		}

		return fields;
	}

	ErrorOr<TCResult2> StructLit::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		const StructType* struct_type = nullptr;
		if(struct_name.name.empty())
		{
			if(infer == nullptr)
			{
				return ErrMsg(ts, "cannot infer type of struct literal");
			}
			else if(infer->isStruct())
			{
				struct_type = infer->toStruct();
			}
			else
			{
				if(infer->isOptional() && infer->optionalElement()->isStruct())
					struct_type = infer->optionalElement()->toStruct();
				else
					return ErrMsg(ts, "inferred non-struct type '{}' for struct literal", infer);
			}
		}
		else
		{
			auto t = TRY(ts->resolveType(frontend::PType::named(this->struct_name)));
			if(not t->isStruct())
				return ErrMsg(ts, "invalid non-struct type '{}' for struct literal", t);

			struct_type = t->toStruct();
		}

		auto struct_defn = dynamic_cast<const cst::StructDefn*>(TRY(ts->getDefinitionForType(struct_type)));
		assert(struct_defn != nullptr);

		auto fields = get_field_things(struct_defn, struct_type);

		// make sure the struct has all the things
		std::vector<ArrangeArg> processed_field_types {};
		std::vector<std::unique_ptr<cst::Expr>> processed_field_exprs {};

		bool saw_named = false;
		for(size_t i = 0; i < this->field_inits.size(); i++)
		{
			auto& f = this->field_inits[i];
			if(f.name.has_value())
				saw_named = true;
			else if(saw_named)
				return ErrMsg(ts, "positional field initialiser not allowed after named field initialiser");

			const Type* infer_type = nullptr;
			if(f.name.has_value() && struct_type->hasFieldNamed(*f.name))
				infer_type = struct_type->getFieldNamed(*f.name);
			else if(not f.name.has_value())
				infer_type = struct_type->getFieldAtIndex(i);

			processed_field_exprs.push_back(TRY(f.value->typecheck2(ts, infer_type)).take_expr());
			processed_field_types.push_back({
			    .type = processed_field_exprs.back()->type(),
			    .name = f.name,
			});
		}

		auto arrangement = TRY(arrangeCallArguments(ts, fields, processed_field_types, "struct", "field", "field"));
		std::vector<Either<std::unique_ptr<cst::Expr>, const cst::Expr*>> final_fields {};

		for(size_t i = 0; i < arrangement.arguments.size(); i++)
		{
			auto& arg = arrangement.arguments[i];
			if(arg.second.is_right())
			{
				final_fields.push_back(Right(arg.second.right()));
				continue;
			}

			auto arg_idx = arg.second.left();
			assert(processed_field_exprs[arg_idx] != nullptr);

			final_fields.push_back(Left(std::move(processed_field_exprs[arg_idx])));
		}

		assert(final_fields.size() == struct_type->getFields().size());

		return TCResult2::ofRValue<cst::StructLit>(m_location, struct_type, std::move(final_fields));
	}
}
