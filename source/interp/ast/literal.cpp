// literal.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp
{
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

	ErrorOr<EvalResult> LengthExpr::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::length(this->length));
	}


	ErrorOr<TCResult> BooleanLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue(Type::makeBool());
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

	ErrorOr<EvalResult> StringLit::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::string(this->string));
	}


	ErrorOr<TCResult> CharLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue(Type::makeChar());
	}

	ErrorOr<EvalResult> CharLit::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::character(this->character));
	}



	ErrorOr<TCResult> NullLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue(Type::makeNullPtr());
	}

	ErrorOr<EvalResult> NullLit::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::nullPointer());
	}
}
