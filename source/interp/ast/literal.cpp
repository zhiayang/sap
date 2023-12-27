// literal.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"
#include "interp/overload_resolution.h"

namespace sap::interp::ast
{
	ErrorOr<std::vector<cst::ExprOrDefaultPtr>> arrange_union_args(Typechecker* ts,
	    const std::vector<ast::ContextIdent::Arg>& case_values,
	    const cst::UnionDefn* union_defn,
	    size_t case_idx,
	    const StructType* case_type)
	{
		ExpectedParams fields {};

		auto& case_fields = case_type->getFields();
		for(size_t i = 0; i < case_fields.size(); i++)
		{
			fields.push_back({
			    .name = case_fields[i].name,
			    .type = case_fields[i].type,
			    .default_value = union_defn->cases[case_idx].default_values[i].get(),
			});
		}

		// TODO: huge code dupe with struct_literal.cpp
		std::vector<InputArg> processed_field_types {};
		std::vector<std::unique_ptr<cst::Expr>> processed_field_exprs {};

		bool saw_named = false;
		for(size_t i = 0; i < case_values.size(); i++)
		{
			auto& f = case_values[i];
			if(f.name.has_value())
				saw_named = true;
			else if(saw_named)
				return ErrMsg(ts, "positional field initialiser not allowed after named field initialiser");

			const Type* infer_type = nullptr;
			if(f.name.has_value() && case_type->hasFieldNamed(*f.name))
				infer_type = case_type->getFieldNamed(*f.name);
			else if(not f.name.has_value())
				infer_type = case_type->getFieldAtIndex(i);

			processed_field_exprs.push_back(TRY(f.value->typecheck(ts, infer_type)).take_expr());
			processed_field_types.push_back({
			    .type = processed_field_exprs.back()->type(),
			    .name = f.name,
			});
		}

		auto arrangement = TRY(arrangeCallArguments(ts, fields, processed_field_types, "case", "field", "field"));
		std::vector<cst::ExprOrDefaultPtr> final_fields {};

		for(size_t i = 0; i < arrangement.arguments.size(); i++)
		{
			auto& arg = arrangement.arguments[i];

			if(arg.value.is_right())
				return ErrMsg(ts, "Invalid use of variadic pack in struct initialiser");

			auto& arg_val = arg.value.left();
			if(arg_val.is_right())
			{
				final_fields.push_back(arg_val.right());
				continue;
			}

			auto arg_idx = arg_val.left();
			assert(processed_field_exprs[arg_idx] != nullptr);

			final_fields.push_back(std::move(processed_field_exprs[arg_idx]));
		}

		assert(final_fields.size() == case_type->getFields().size());
		return OkMove(final_fields);
	}

	ErrorOr<TCResult> ContextIdent::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		// note: we allow this to resolve both a struct field and an enumerator, preferring the field.
		// if both exist, we raise an error.
		if(ts->haveStructFieldContext())
		{
			auto struct_ctx = ts->getStructFieldContext();
			auto struct_defn = dynamic_cast<const cst::StructDefn*>(TRY(ts->getDefinitionForType(struct_ctx)));
			assert(struct_defn != nullptr);

			if(struct_ctx->hasFieldNamed(this->name))
			{
				// check if the inferred type is an enum, and whether that enum has an enumerator with our name
				if(infer && (infer->isEnum() || (infer->isOptional() && infer->optionalElement()->isEnum())))
				{
					auto enum_type = (infer->isEnum() ? infer->toEnum() : infer->optionalElement()->toEnum());
					auto enum_defn = dynamic_cast<const cst::EnumDefn*>(TRY(ts->getDefinitionForType(enum_type)));
					assert(enum_defn != nullptr);

					if(enum_defn->getEnumeratorNamed(this->name))
					{
						return ErrMsg(ts,
						    "ambiguous use of '.{}': could be enumerator of '{}' or field of struct '{}'", //
						    this->name, enum_defn->declaration->name, struct_defn->declaration->name);
					}
				}

				auto field_type = struct_ctx->getFieldNamed(this->name);
				return TCResult::ofRValue<cst::DotOp>(m_location, field_type, /*is_optional: */ false, struct_ctx,
				    std::make_unique<cst::StructContextSelf>(m_location, struct_ctx), this->name);
			}
		}

		const EnumType* enum_type = nullptr;
		const UnionType* union_type = nullptr;

		if(infer == nullptr)
			return ErrMsg(ts, "cannot infer type for context-identifier");

		else if(infer->isEnum())
			enum_type = infer->toEnum();
		else if(infer->isOptional() && infer->optionalElement()->isEnum())
			enum_type = infer->optionalElement()->toEnum();
		else if(infer->isUnion())
			union_type = infer->toUnion();
		else if(infer->isOptional() && infer->optionalElement()->isUnion())
			union_type = infer->optionalElement()->toUnion();
		else
			return ErrMsg(ts, "inferred invalid type '{}' for context-identifier", infer);

		assert(enum_type || union_type);
		if(enum_type != nullptr)
		{
			auto enum_defn = static_cast<const cst::EnumDefn*>(TRY(ts->getDefinitionForType(enum_type)));
			auto enumerator = enum_defn->getEnumeratorNamed(this->name);

			if(enumerator == nullptr)
			{
				return ErrMsg(ts, "inferred enumeration '{}' for context-identifier '.{}' has no such enumerator",
				    enum_defn->declaration->name, this->name);
			}

			return TCResult::ofLValue(std::make_unique<cst::EnumeratorExpr>(m_location, enum_type, enumerator),
			    /* mutable: */ false);
		}
		else
		{
			assert(union_type != nullptr);
			auto union_defn = static_cast<const cst::UnionDefn*>(TRY(ts->getDefinitionForType(union_type)));
			if(not union_type->hasCaseNamed(this->name))
			{
				return ErrMsg(ts, "inferred union '{}' for context-identifier has no case named '{}'",
				    union_defn->declaration->name, this->name);
			}

			auto case_idx = union_type->getCaseIndex(this->name);
			auto case_values = TRY(arrange_union_args(ts, this->arguments, union_defn, case_idx,
			    union_type->getCaseAtIndex(case_idx)));

			return TCResult::ofRValue<cst::UnionExpr>(m_location, union_type, union_defn, case_idx,
			    std::move(case_values));
		}
	}

	ErrorOr<TCResult> ArrayLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		if(elem_type.has_value())
		{
			auto ty = TRY(ts->resolveType(*elem_type));
			if(ty->isVoid())
				return ErrMsg(ts, "cannot make an array with element type 'void'");

			std::vector<std::unique_ptr<cst::Expr>> elms;
			for(auto& elm : this->elements)
			{
				auto et = elms.emplace_back(TRY(elm->typecheck(ts, ty)).take_expr())->type();
				if(et != ty)
					return ErrMsg(elm->loc(), "mismatched types in array literal: expected '{}', got '{}'", ty, et);
			}

			return TCResult::ofRValue<cst::ArrayLit>(m_location, Type::makeArray(ty), std::move(elms));
		}
		else
		{
			std::vector<std::unique_ptr<cst::Expr>> elms;

			// note: void array is a special case.
			if(this->elements.empty())
				return TCResult::ofRValue<cst::ArrayLit>(m_location, Type::makeArray(Type::makeVoid()),
				    std::move(elms));

			auto et = infer ? infer->arrayElement() : nullptr;
			et = elms.emplace_back(TRY(this->elements[0]->typecheck(ts, et)).take_expr())->type();

			for(size_t i = 1; i < this->elements.size(); i++)
			{
				auto t2 = elms.emplace_back(TRY(this->elements[i]->typecheck(ts, et)).take_expr())->type();
				if(t2 != et)
				{
					return ErrMsg(this->elements[i]->loc(),
					    "mismatched types in array literal: expected '{}', got '{}'", et, t2);
				}
			}

			return TCResult::ofRValue<cst::ArrayLit>(m_location, Type::makeArray(et), std::move(elms));
		}
	}

	ErrorOr<TCResult> LengthExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue<cst::LengthExpr>(m_location, this->length);
	}

	ErrorOr<TCResult> BooleanLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue<cst::BooleanLit>(m_location, this->value);
	}

	ErrorOr<TCResult> NumberLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto ret = std::make_unique<cst::NumberLit>(m_location,
		    this->is_floating ? Type::makeFloating() : Type::makeInteger(), this->is_floating);

		if(this->is_floating)
			ret->float_value = this->float_value;
		else
			ret->int_value = this->int_value;

		return TCResult::ofRValue(std::move(ret));
	}

	ErrorOr<TCResult> StringLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue<cst::StringLit>(m_location, this->string);
	}

	ErrorOr<TCResult> CharLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue<cst::CharLit>(m_location, this->character);
	}

	ErrorOr<TCResult> NullLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue<cst::NullLit>(m_location);
	}
}
