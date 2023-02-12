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

			auto t1 = TRY(this->elements[0]->typecheck(ts)).type();
			for(size_t i = 1; i < this->elements.size(); i++)
			{
				auto t2 = TRY(this->elements[i]->typecheck(ts, t1)).type();
				if(t2 != t1)
					return ErrMsg(this->elements[i]->loc(),
						"mismatched types in array literal: expected '{}', got '{}'", t1, t2);
			}

			return TCResult::ofRValue(Type::makeArray(t1));
		}
	}

	ErrorOr<EvalResult> ArrayLit::evaluate_impl(Evaluator* ev) const
	{
		std::vector<Value> values {};
		for(auto& elm : this->elements)
			values.push_back(TRY_VALUE(elm->evaluate(ev)));

		return EvalResult::ofValue(Value::array(this->get_type()->toArray()->elementType(), std::move(values)));
	}






	ErrorOr<TCResult> EnumLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		const EnumType* enum_type = nullptr;

		if(infer == nullptr)
		{
			return ErrMsg(ts, "cannot infer type for enum literal");
		}
		else if(infer->isEnum())
		{
			enum_type = infer->toEnum();
		}
		else
		{
			if(infer->isOptional() && infer->optionalElement()->isEnum())
				enum_type = infer->optionalElement()->toEnum();
			else
				return ErrMsg(ts, "inferred non-enum type '{}' for enum literal", infer);
		}

		auto enum_defn = dynamic_cast<const EnumDefn*>(TRY(ts->getDefinitionForType(enum_type)));
		assert(enum_defn != nullptr);

		auto enumerator = enum_defn->getEnumeratorNamed(this->name);

		if(enumerator == nullptr)
			return ErrMsg(ts, "enum '{}' has no enumerator named '{}'", (const Type*) enum_type, this->name);

		m_enumerator_defn = enumerator;
		return TCResult::ofLValue(enum_type, /* mutable: */ false);
	}

	ErrorOr<EvalResult> EnumLit::evaluate_impl(Evaluator* ev) const
	{
		assert(m_enumerator_defn != nullptr);
		return EvalResult::ofLValue(*ev->getGlobalValue(m_enumerator_defn));
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



	ErrorOr<TCResult> NullLit::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue(Type::makeNullPtr());
	}

	ErrorOr<EvalResult> NullLit::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofValue(Value::nullPointer());
	}
}
