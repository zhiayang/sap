// enum.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> EnumDefn::EnumeratorDecl::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(infer != nullptr && infer->isEnum());

		TRY(ts->current()->declare(this));
		return TCResult::ofRValue(infer);
	}

	ErrorOr<TCResult> EnumDefn::EnumeratorDefn::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(infer != nullptr && infer->isEnum());

		this->declaration->resolve(this);
		auto enum_type = TRY(this->declaration->typecheck(ts, infer)).type()->toEnum();
		auto elm_type = enum_type->elementType();

		if(m_value != nullptr)
		{
			auto ty = TRY(m_value->typecheck(ts, elm_type)).type();
			if(not ts->canImplicitlyConvert(ty, elm_type))
				return ErrMsg(ts, "cannot use value of type '{}' as enumerator for enum type '{}'", ty, (const Type*) enum_type);
		}
		else if(not elm_type->isInteger())
		{
			return ErrMsg(ts, "non-integral enumerators must explicitly specify a value");
		}

		return TCResult::ofRValue(enum_type);
	}




	ErrorOr<TCResult> EnumDecl::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(infer != nullptr);

		auto enum_type = Type::makeEnum(this->name, infer);

		TRY(ts->current()->declare(this));
		return TCResult::ofRValue(enum_type);
	}

	ErrorOr<TCResult> EnumDefn::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto enum_elm_type = TRY(ts->resolveType(m_enumerator_type));
		this->declaration->resolve(this);

		auto enum_type = TRY(this->declaration->typecheck(ts, enum_elm_type)).type()->toEnum();
		TRY(ts->addTypeDefinition(enum_type, this));

		auto scope = ts->current()->lookupOrDeclareNamespace(this->declaration->name);
		auto _ = ts->pushTree(scope);

		util::hashset<std::string> enum_names {};

		for(auto& enumerator : m_enumerators)
		{
			if(enum_names.contains(enumerator.declaration->name))
				return ErrMsg(ts, "duplicate enumerator '{}'", enumerator.declaration->name);

			enum_names.insert(enumerator.declaration->name);
			TRY(enumerator.typecheck(ts, enum_type));
		}

		m_resolved_enumerator_type = enum_type;
		return TCResult::ofRValue(enum_type);
	}





	ErrorOr<EvalResult> EnumDecl::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> EnumDefn::evaluate_impl(Evaluator* ev) const
	{
		if(m_resolved_enumerator_type->elementType()->isInteger())
		{
			int64_t prev_value = -1;
			for(auto& e : m_enumerators)
				TRY(e.evaluate_impl(ev, &prev_value));
		}
		else
		{
			for(auto& enumerator : m_enumerators)
				TRY(enumerator.evaluate(ev));
		}

		return EvalResult::ofVoid();
	}



	ErrorOr<EvalResult> EnumDefn::EnumeratorDecl::evaluate_impl(Evaluator* ev) const
	{
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> EnumDefn::EnumeratorDefn::evaluate_impl(Evaluator* ev) const
	{
		assert(m_value != nullptr);

		auto value = ev->castValue(TRY_VALUE(m_value->evaluate(ev)), this->get_type());
		ev->frame().setValue(this, std::move(value));

		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> EnumDefn::EnumeratorDefn::evaluate_impl(Evaluator* ev, int64_t* prev_value) const
	{
		auto et = this->get_type()->toEnum();
		if(m_value == nullptr)
		{
			assert(et->elementType()->isInteger());
			ev->frame().setValue(this, Value::enumerator(et, Value::integer(++(*prev_value))));
		}
		else
		{
			auto tmp = TRY_VALUE(m_value->evaluate(ev));
			if(tmp.isInteger())
				(*prev_value) = tmp.getInteger();

			auto value = Value::enumerator(et, ev->castValue(std::move(tmp), et->elementType()));
			ev->frame().setValue(this, std::move(value));
		}

		return EvalResult::ofVoid();
	}
}
