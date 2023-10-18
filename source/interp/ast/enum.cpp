// enum.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
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
		auto enum_type = this->declaration->type->toEnum();
		auto enum_elm_type = enum_type->elementType();

		std::vector<std::unique_ptr<cst::EnumeratorDefn>> enums {};
		for(auto& e : m_enumerators)
		{
			auto ce = std::make_unique<cst::EnumeratorDefn>(e.location, e.declaration,
			    enums.empty() ? nullptr : enums.back().get());

			if(e.value != nullptr)
			{
				auto ty = TRY(e.value->typecheck2(ts, enum_elm_type)).type();
				if(not ts->canImplicitlyConvert(ty, enum_elm_type))
				{
					return ErrMsg(ts, "cannot use value of type '{}' as enumerator for enum type '{}'", ty,
					    (const Type*) enum_type);
				}
			}
			else if(not enum_elm_type->isInteger())
			{
				return ErrMsg(ts, "non-integral enumerators must explicitly specify a value");
			}

			enums.push_back(std::move(ce));
		}

		auto defn = std::make_unique<cst::EnumDefn>(m_location, this->declaration, std::move(enums));
		this->declaration->define(defn.get());

		TRY(ts->addTypeDefinition(enum_type, defn.get()));
		return TCResult2::ofVoid(std::move(defn));
	}
}
