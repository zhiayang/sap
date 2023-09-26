// enum.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
#if 0
	ErrorOr<void> EnumeratorDefn::declare(Typechecker* ts) const
	{
		// if we have neither, it's an error
		if(not this->explicit_type.has_value() && this->initialiser == nullptr)
			return ErrMsg(ts, "variable without explicit type must have an initialiser");

		auto the_type = TRY([&]() -> ErrorOr<TCResult> {
			if(this->explicit_type.has_value())
			{
				auto resolved_type = TRY(ts->resolveType(*this->explicit_type));

				if(resolved_type->isVoid())
					return ErrMsg(ts, "cannot declare variable of type 'void'");

				if(this->initialiser != nullptr)
				{
					auto initialiser_type = TRY(this->initialiser->typecheck(ts, /* infer: */ resolved_type)).type();
					if(not ts->canImplicitlyConvert(initialiser_type, resolved_type))
					{
						return ErrMsg(ts, "cannot initialise variable of type '{}' with expression of type '{}'", //
						    resolved_type, initialiser_type);
					}
				}

				return TCResult::ofRValue(resolved_type);
			}
			else
			{
				assert(this->initialiser != nullptr);
				return this->initialiser->typecheck(ts);
			}
		}());

		auto decl = cst::Declaration(m_location, ts->current(), this->name, the_type.type(), this->is_mutable);
		this->declaration = TRY(ts->current()->declare(std::move(decl)));

		return Ok();
	}

	ErrorOr<TCResult2> EnumeratorDefn::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
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

		return TCResult2::ofVoid<cst::EnumeratorDefn>(m_location, this->declaration->name, enum_type);
	}
#endif


	ErrorOr<void> EnumDefn::declare(Typechecker* ts) const
	{
		auto enum_elm_type = TRY(ts->resolveType(m_enumerator_type));
		auto enum_type = Type::makeEnum(ts->current()->scopedName(this->name), enum_elm_type);

		// move into the new scope to declare the enumerators
		auto scope = ts->current()->lookupOrDeclareNamespace(this->name);
		{
			auto _ = ts->pushTree(scope);

			util::hashset<std::string> enum_names {};


			for(auto& enumerator : m_enumerators)
			{
				if(enum_names.contains(enumerator.name))
					return ErrMsg(ts, "duplicate enumerator '{}'", enumerator.name);

				enum_names.insert(enumerator.name);
				enumerator.declaration = TRY(scope->declare(cst::Declaration(enumerator.location, ts->current(),
				    enumerator.name, enum_type, /* mutable: */ false)));
			}
		}

		// declare the enum itself now
		auto decl = cst::Declaration(m_location, ts->current(), this->name, enum_type,
		    /* mutable: */ false);

		this->declaration = TRY(ts->current()->declare(std::move(decl)));
		return Ok();
	}

	ErrorOr<TCResult2> EnumDefn::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(this->declaration != nullptr);


		return TCResult2::ofVoid<cst::EnumDefn>(m_location, this->declaration->name, enum_type, std::move(enums));
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
