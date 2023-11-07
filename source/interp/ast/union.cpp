// union.cpp
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"
#include "interp/overload_resolution.h"

namespace sap::interp::ast
{
	ErrorOr<void> UnionDefn::declare(Typechecker* ts) const
	{
		// same as struct, we just fill in the cases later.
		auto union_type = Type::makeUnion(ts->current()->scopedName(this->name), {});
		auto decl = cst::Declaration(m_location, ts->current(), this->name, union_type,
		    /* mutable: */ false);

		this->declaration = TRY(ts->current()->declare(std::move(decl)));
		return Ok();
	}

	ErrorOr<TCResult> UnionDefn::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(this->declaration != nullptr);
		auto union_type = this->declaration->type->toUnion();

		util::hashset<std::string> seen_names {};

		std::vector<UnionType::Case> case_types {};
		std::vector<cst::UnionDefn::Case> defn_cases {};

		for(auto& case_ : this->cases)
		{
			if(seen_names.contains(case_.name))
				return ErrMsg(ts, "duplicate case '{}' in union '{}'", case_.name, this->name);

			seen_names.insert(case_.name);

			auto case_name = union_type->name().parentScopeFor(case_.name);
			std::vector<std::pair<std::string, const Type*>> case_params {};
			std::vector<std::unique_ptr<cst::Expr>> default_values {};

			// make sure the user didn't clown by duplicating param names
			util::hashset<std::string> seen_params {};
			for(auto& p : case_.params)
			{
				if(seen_params.contains(p.name))
					return ErrMsg(ts, "duplicate parameter name '{}' in case '{}'", p.name, case_.name);

				seen_params.insert(p.name);

				auto param_type = TRY(ts->resolveType(p.type));
				case_params.push_back({ p.name, param_type });

				if(p.default_value != nullptr)
				{
					auto def_value = TRY(p.default_value->typecheck(ts, param_type)).take_expr();
					if(not ts->canImplicitlyConvert(def_value->type(), param_type))
					{
						return ErrMsg(ts, "cannot initialise parameter of type '{}' with value of type '{}'",
						    param_type, def_value->type());
					}

					default_values.push_back(std::move(def_value));
				}
				else
				{
					default_values.emplace_back();
				}
			}

			// ok, here we need to invent a struct type for each case of the union.
			auto case_type = Type::makeStruct(std::move(case_name), std::move(case_params));
			case_types.push_back(UnionType::Case {
			    .name = case_.name,
			    .type = case_type,
			});

			defn_cases.push_back(cst::UnionDefn::Case {
			    .name = case_.name,
			    .type = case_type,
			    .default_values = std::move(default_values),
			});
		}

		const_cast<UnionType*>(union_type)->setCases(std::move(case_types));
		auto defn = std::make_unique<cst::UnionDefn>(m_location, this->declaration, std::move(defn_cases));

		this->declaration->define(defn.get());
		TRY(ts->addTypeDefinition(union_type, defn.get()));

		return TCResult::ofVoid(std::move(defn));
	}
}
