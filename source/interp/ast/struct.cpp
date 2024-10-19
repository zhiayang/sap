// struct.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"
#include "interp/overload_resolution.h"

namespace sap::interp::ast
{
	ErrorOr<void> StructDefn::declare(Typechecker* ts) const
	{
		// when declaring a struct, we just make an empty field list.
		// the fields will be set later on.
		auto struct_type = Type::makeStruct(ts->current()->scopedName(this->name), {});
		auto decl = cst::Declaration(m_location, ts->current(), this->name, struct_type,
		    /* mutable: */ false);

		this->declaration = TRY(ts->current()->declare(std::move(decl)));
		return Ok();
	}

	ErrorOr<TCResult> StructDefn::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(this->declaration != nullptr);
		auto struct_type = this->declaration->type->toStruct();

		util::hashset<std::string> seen_names {};
		std::vector<cst::StructDefn::Field> defn_fields {};
		std::vector<StructType::Field> type_fields {};

		for(auto& field : m_fields)
		{
			if(seen_names.contains(field.name))
				return ErrMsg(ts, "duplicate field '{}' in struct '{}'", field.name, this->name);

			seen_names.insert(field.name);
			auto field_type = TRY(ts->resolveType(field.type));
			if(field_type == struct_type)
				return ErrMsg(ts, "field '{}' cannot have the same type as its containing struct", field.name);
			else if(field_type->isVoid())
				return ErrMsg(ts, "field '{}' cannot have 'void' type", field.name);

			std::unique_ptr<cst::Expr> init_value;
			if(field.initialiser != nullptr)
			{
				init_value = TRY(field.initialiser->typecheck(ts, field_type)).take_expr();
				if(not ts->canImplicitlyConvert(init_value->type(), field_type))
				{
					return ErrMsg(ts, "cannot initialise field of type '{}' with value of type '{}'", field_type,
					    init_value->type());
				}
			}

			type_fields.push_back(StructType::Field {
			    .name = field.name,
			    .type = field_type,
			});

			defn_fields.push_back(cst::StructDefn::Field {
			    .name = field.name,
			    .type = field_type,
			    .initialiser = std::move(init_value),
			});
		}

		const_cast<StructType*>(struct_type)->setFields(std::move(type_fields));

		auto defn = std::make_unique<cst::StructDefn>(m_location, this->declaration, std::move(defn_fields));

		this->declaration->define(defn.get());
		TRY(ts->addTypeDefinition(struct_type, defn.get()));

		return TCResult::ofVoid(std::move(defn));
	}



	ErrorOr<TCResult> StructUpdateOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lres = TRY(this->structure->typecheck(ts, infer));
		if(this->is_assignment && not lres.isLValue())
			return ErrMsg(ts, "cannot assign to non-lvalue");
		else if(this->is_assignment && not lres.isMutable())
			return ErrMsg(ts, "cannot assign to immutable lvalue");

		auto lhs = std::move(lres).take_expr();
		if(not lhs->type()->isStruct())
			return ErrMsg(ts, "left-hand side of '//' operator must be a struct; got '{}'", lhs->type());

		auto struct_type = lhs->type()->toStruct();
		auto _ = ts->pushStructFieldContext(struct_type);

		std::vector<cst::StructUpdateOp::Update> update_exprs;
		for(auto& [name, expr] : this->updates)
		{
			if(not struct_type->hasFieldNamed(name))
				return ErrMsg(expr->loc(), "struct '{}' has no field named '{}'", struct_type->name().name, name);

			auto field_type = struct_type->getFieldNamed(name);
			auto new_expr = TRY(expr->typecheck(ts, field_type)).take_expr();

			// check if the provided field is actually optional and error if it's not
			if(this->is_optional && not field_type->isOptional())
				return ErrMsg(expr->loc(), "cannot update non-optional struct field '{}' using '//?' operator", name);

			if(not ts->canImplicitlyConvert(new_expr->type(), field_type))
			{
				return ErrMsg(expr->loc(), "cannot update struct field '{}' with type '{}' using a value of type '{}'",
				    name, field_type, new_expr->type());
			}

			update_exprs.push_back({
			    .field = name,
			    .expr = std::move(new_expr),
			});
		}

		return TCResult::ofRValue<cst::StructUpdateOp>(m_location, struct_type, std::move(lhs), std::move(update_exprs),
		    this->is_assignment, this->is_optional);
	}
}
