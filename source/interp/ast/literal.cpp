// literal.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
#if 0
	ErrorOr<TCResult> ContextIdent::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		// note: we allow this to resolve both a struct field and an enumerator, preferring the field.
		// if both exist, we raise an error.
		if(ts->haveStructFieldContext())
		{
			auto struct_ctx = ts->getStructFieldContext();
			auto struct_defn = dynamic_cast<const StructDefn*>(TRY(ts->getDefinitionForType(struct_ctx)));
			assert(struct_defn != nullptr);

			if(struct_ctx->hasFieldNamed(this->name))
			{
				// check if the inferred type is an enum, and whether that enum has an enumerator with our name
				if(infer && (infer->isEnum() || (infer->isOptional() && infer->optionalElement()->isEnum())))
				{
					auto enum_type = (infer->isEnum() ? infer->toEnum() : infer->optionalElement()->toEnum());
					auto enum_defn = dynamic_cast<const EnumDefn*>(TRY(ts->getDefinitionForType(enum_type)));
					assert(enum_defn != nullptr);

					if(enum_defn->getEnumeratorNamed(this->name))
					{
						return ErrMsg(ts,
						    "ambiguous use of '.{}': could be enumerator of '{}' or field of struct '{}'", //
						    this->name, enum_defn->declaration->name, struct_defn->declaration->name);
					}
				}

				// return the type of the field.
				m_context = struct_ctx;
				return TCResult::ofRValue(struct_ctx->getFieldNamed(this->name));
			}
		}

		const EnumType* enum_type = nullptr;

		if(infer == nullptr)
			return ErrMsg(ts, "cannot infer type for context-sensitive identifier");

		else if(infer->isEnum())
			enum_type = infer->toEnum();

		else if(infer->isOptional() && infer->optionalElement()->isEnum())
			enum_type = infer->optionalElement()->toEnum();
		else
			return ErrMsg(ts, "inferred invalid type type '{}' for context-sensitive identifier", infer);

		auto enum_defn = dynamic_cast<const EnumDefn*>(TRY(ts->getDefinitionForType(enum_type)));
		assert(enum_defn != nullptr);

		auto enumerator = enum_defn->getEnumeratorNamed(this->name);

		if(enumerator == nullptr)
		{
			return ErrMsg(ts, "inferred enumeration '{}' for context-sensitive identifier '.{}' has no such enumerator",
			    enum_defn->declaration->name, this->name);
		}

		m_context = enumerator;
		return TCResult::ofLValue(enum_type, /* mutable: */ false);
	}
#endif


	ErrorOr<TCResult2> ContextIdent::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		// note: we allow this to resolve both a struct field and an enumerator, preferring the field.
		// if both exist, we raise an error.
		if(ts->haveStructFieldContext())
		{
			auto struct_ctx = ts->getStructFieldContext();
			auto struct_defn = dynamic_cast<const StructDefn*>(TRY(ts->getDefinitionForType(struct_ctx)));
			assert(struct_defn != nullptr);

			if(struct_ctx->hasFieldNamed(this->name))
			{
				// check if the inferred type is an enum, and whether that enum has an enumerator with our name
				if(infer && (infer->isEnum() || (infer->isOptional() && infer->optionalElement()->isEnum())))
				{
					auto enum_type = (infer->isEnum() ? infer->toEnum() : infer->optionalElement()->toEnum());
					auto enum_defn = dynamic_cast<const EnumDefn*>(TRY(ts->getDefinitionForType(enum_type)));
					assert(enum_defn != nullptr);

					if(enum_defn->getEnumeratorNamed(this->name))
					{
						return ErrMsg(ts,
						    "ambiguous use of '.{}': could be enumerator of '{}' or field of struct '{}'", //
						    this->name, enum_defn->name, struct_defn->name);
					}
				}

				auto field_type = struct_ctx->getFieldNamed(this->name);
				return TCResult2::ofRValue<cst::DotOp>(m_location, field_type, /*is_optional: */ false, struct_ctx,
				    std::make_unique<cst::StructContextSelf>(m_location, struct_ctx), this->name);
			}
		}

		const EnumType* enum_type = nullptr;

		if(infer == nullptr)
			return ErrMsg(ts, "cannot infer type for context-sensitive identifier");

		else if(infer->isEnum())
			enum_type = infer->toEnum();

		else if(infer->isOptional() && infer->optionalElement()->isEnum())
			enum_type = infer->optionalElement()->toEnum();
		else
			return ErrMsg(ts, "inferred invalid type type '{}' for context-sensitive identifier", infer);

		auto enum_defn = dynamic_cast<const EnumDefn*>(TRY(ts->getDefinitionForType(enum_type)));
		assert(enum_defn != nullptr);

		auto enumerator = enum_defn->getEnumeratorNamed(this->name);

		if(enumerator == nullptr)
		{
			return ErrMsg(ts, "inferred enumeration '{}' for context-sensitive identifier '.{}' has no such enumerator",
			    enum_defn->name, this->name);
		}

		return TCResult2::ofLValue(std::make_unique<cst::EnumeratorExpr>(m_location, enum_type), /* mutable: */ false);
	}








#if 0
	ErrorOr<EvalResult> ContextIdent::evaluate_impl(Evaluator* ev) const
	{
		if(auto enumerator_def = std::get_if<const EnumDefn::EnumeratorDefn*>(&m_context))
		{
			return EvalResult::ofLValue(*ev->getGlobalValue(*enumerator_def));
		}
		else if(auto struct_type = std::get_if<const StructType*>(&m_context))
		{
			auto& struct_value = ev->getStructFieldContext();

			assert((*struct_type)->hasFieldNamed(this->name));
			assert(struct_value.type() == *struct_type);

			return EvalResult::ofValue(struct_value.getStructField((*struct_type)->getFieldIndex(this->name)).clone());
		}
		else
		{
			sap::internal_error("aaa");
		}
	}
#endif




	ErrorOr<TCResult> ArrayLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		if(elem_type.has_value())
		{
			auto ty = TRY(ts->resolveType(*elem_type));
			if(ty->isVoid())
				return ErrMsg(ts, "cannot make an array with element type 'void'");

			for(auto& elm : this->elements)
			{
				auto et = TRY(elm->typecheck(ts, ty)).type();
				if(et != ty)
					return ErrMsg(elm->loc(), "mismatched types in array literal: expected '{}', got '{}'", ty, et);
			}

			return TCResult::ofRValue(Type::makeArray(ty));
		}
		else
		{
			// note: void array is a special case.
			if(this->elements.empty())
				return TCResult::ofRValue(Type::makeArray(Type::makeVoid()));

			auto et = infer ? infer->arrayElement() : nullptr;
			et = TRY(this->elements[0]->typecheck(ts, et)).type();

			for(size_t i = 1; i < this->elements.size(); i++)
			{
				auto t2 = TRY(this->elements[i]->typecheck(ts, et)).type();
				if(t2 != et)
					return ErrMsg(this->elements[i]->loc(),
					    "mismatched types in array literal: expected '{}', got '{}'", et, t2);
			}

			return TCResult::ofRValue(Type::makeArray(et));
		}
	}

	ErrorOr<TCResult2> ArrayLit::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		if(elem_type.has_value())
		{
			auto ty = TRY(ts->resolveType(*elem_type));
			if(ty->isVoid())
				return ErrMsg(ts, "cannot make an array with element type 'void'");

			std::vector<std::unique_ptr<cst::Expr>> elms;
			for(auto& elm : this->elements)
			{
				auto et = elms.emplace_back(TRY(elm->typecheck2(ts, ty)).take_expr())->type();
				if(et != ty)
					return ErrMsg(elm->loc(), "mismatched types in array literal: expected '{}', got '{}'", ty, et);
			}

			return TCResult2::ofRValue<cst::ArrayLit>(m_location, Type::makeArray(ty), std::move(elms));
		}
		else
		{
			std::vector<std::unique_ptr<cst::Expr>> elms;

			// note: void array is a special case.
			if(this->elements.empty())
				return TCResult2::ofRValue<cst::ArrayLit>(m_location, Type::makeArray(Type::makeVoid()),
				    std::move(elms));

			auto et = infer ? infer->arrayElement() : nullptr;
			et = elms.emplace_back(TRY(this->elements[0]->typecheck2(ts, et)).take_expr())->type();

			for(size_t i = 1; i < this->elements.size(); i++)
			{
				auto t2 = elms.emplace_back(TRY(this->elements[i]->typecheck2(ts, et)).take_expr())->type();
				if(t2 != et)
				{
					return ErrMsg(this->elements[i]->loc(),
					    "mismatched types in array literal: expected '{}', got '{}'", et, t2);
				}
			}

			return TCResult2::ofRValue<cst::ArrayLit>(m_location, Type::makeArray(et), std::move(elms));
		}
	}

	ErrorOr<EvalResult> ArrayLit::evaluate_impl(Evaluator* ev) const
	{
		std::vector<Value> values {};
		for(auto& elm : this->elements)
			values.push_back(TRY_VALUE(elm->evaluate(ev)));

		return EvalResult::ofValue(Value::array(this->get_type()->toArray()->elementType(), std::move(values)));
	}



	ErrorOr<TCResult> LengthExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue(Type::makeLength());
	}

	ErrorOr<TCResult2> LengthExpr::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult2::ofRValue<cst::LengthExpr>(m_location, this->length);
	}

	ErrorOr<EvalResult> LengthExpr::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::length(this->length));
	}


	ErrorOr<TCResult> BooleanLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue(Type::makeBool());
	}

	ErrorOr<TCResult2> BooleanLit::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult2::ofRValue<cst::BooleanLit>(m_location, this->value);
	}

	ErrorOr<EvalResult> BooleanLit::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::boolean(this->value));
	}


	ErrorOr<TCResult> NumberLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		if(this->is_floating)
			return TCResult::ofRValue(Type::makeFloating());
		else
			return TCResult::ofRValue(Type::makeInteger());
	}

	ErrorOr<TCResult2> NumberLit::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto ret = std::make_unique<cst::NumberLit>(m_location,
		    this->is_floating ? Type::makeFloating() : Type::makeInteger());

		if(this->is_floating)
			ret->float_value = this->float_value;
		else
			ret->int_value = this->int_value;

		return TCResult2::ofRValue(std::move(ret));
	}

	ErrorOr<EvalResult> NumberLit::evaluate_impl(Evaluator* ev) const
	{
		if(this->is_floating)
			return EvalResult::ofValue(Value::floating(float_value));
		else
			return EvalResult::ofValue(Value::integer(int_value));
	}


	ErrorOr<TCResult> StringLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue(Type::makeString());
	}

	ErrorOr<TCResult2> StringLit::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult2::ofRValue<cst::StringLit>(m_location, this->string);
	}

	ErrorOr<EvalResult> StringLit::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::string(this->string));
	}


	ErrorOr<TCResult> CharLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue(Type::makeChar());
	}

	ErrorOr<TCResult2> CharLit::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult2::ofRValue<cst::CharLit>(m_location, this->character);
	}

	ErrorOr<EvalResult> CharLit::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::character(this->character));
	}



	ErrorOr<TCResult> NullLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue(Type::makeNullPtr());
	}

	ErrorOr<TCResult2> NullLit::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult2::ofRValue<cst::NullLit>(m_location);
	}

	ErrorOr<EvalResult> NullLit::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::nullPointer());
	}
}
