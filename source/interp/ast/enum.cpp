// enum.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult>
	EnumDefn::EnumeratorDecl::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(infer != nullptr && infer->isEnum());

		TRY(ts->current()->declare(this));
		return TCResult::ofRValue(infer);
	}

	ErrorOr<TCResult>
	EnumDefn::EnumeratorDefn::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(infer != nullptr && infer->isEnum());

		this->declaration->resolve(this);
		auto enum_type = TRY(this->declaration->typecheck(ts, infer)).type()->toEnum();
		auto elm_type = enum_type->elementType();

		if(m_value != nullptr)
		{
			auto ty = TRY(m_value->typecheck(ts, elm_type)).type();
			if(not ts->canImplicitlyConvert(ty, elm_type))
				return ErrMsg(ts, "cannot use value of type '{}' as enumerator for enum type '{}'", ty,
				    (const Type*) enum_type);
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
		TRY(ts->current()->declare(this));

		auto enum_type = Type::makeEnum(m_declared_tree->scopeName(this->name), infer);
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

	auto EnumDefn::getEnumeratorNamed(const std::string& name) const -> const EnumeratorDefn*
	{
		for(auto& e : m_enumerators)
		{
			if(e.declaration->name == name)
				return &e;
		}

		return nullptr;
	}
}
