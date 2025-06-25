// if_let.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> IfLetOptionalStmt::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		// make a new scope for the variable binding
		auto tree = ts->current()->declareAnonymousNamespace();

		std::unique_ptr<cst::IfLetOptionalStmt> if_stmt {};
		{
			// push this scope in a block
			auto _ = ts->pushTree(tree);

			auto rhs = TRY(this->expr->typecheck(ts)).take_expr();
			if(not rhs->type()->isOptional())
				return ErrMsg(rhs->loc(), "expected optional type for expression, found '{}'", rhs->type());

			auto var_type = rhs->type()->optionalElement();
			auto decl_ = cst::Declaration(m_location, ts->current(), this->name, var_type, this->is_mutable);
			auto decl = TRY(ts->current()->declare(std::move(decl_)));

			auto var_defn = std::make_unique<cst::VariableDefn>(m_location, decl, /* global: */ false, nullptr,
			    this->is_mutable);

			decl->define(var_defn.get());
			if_stmt = std::make_unique<cst::IfLetOptionalStmt>(m_location, std::move(rhs), std::move(var_defn));
			if_stmt->true_case = TRY(this->true_case->typecheck(ts)).take_block();

			// the new scope is popped here
		}

		if(this->else_case)
			if_stmt->else_case = TRY(this->else_case->typecheck(ts)).take_block();

		return TCResult::ofVoid(std::move(if_stmt));
	}

	ErrorOr<TCResult> IfLetUnionStmt::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		// make a new scope for the variable binding
		auto tree = ts->current()->declareAnonymousNamespace();

		std::unique_ptr<cst::IfLetUnionStmt> if_stmt {};
		{
			// push this scope in a block
			auto _ = ts->pushTree(tree);

			// this is a bit weird, but basically if we have a fully qualified name for the union variant,
			// then we should be able to know what the union itself is, so we can use that to infer the rhs
			const Type* rhs_infer = nullptr;
			if(not this->variant_name.parents.empty())
			{
				// the name itself should be the variant case, so yeet it and try to find a type with that name
				auto tmp = this->variant_name.withoutLastComponent();
				auto union_type = TRY(ts->resolveType(frontend::PType::named(tmp)));
				if(not union_type->isUnion())
					return ErrMsg(m_location, "'{}' did not name a union", this->variant_name);

				rhs_infer = union_type;
			}

			auto rhs = TRY(this->expr->typecheck(ts, /* infer: */ rhs_infer)).take_expr();
			if(not rhs->type()->isUnion())
				return ErrMsg(rhs->loc(), "expected union type for expression, found '{}'", rhs->type());

			const auto union_type = rhs->type()->toUnion();

			// make sure (if we have an infer) that it's the same type. this checks for a potential
			// "loophole" for if the union from the qualified variant name does not match the actual union type
			if(rhs_infer && union_type != rhs_infer)
			{
				return ErrMsg(rhs->loc(), "mismatched union types: '{}' in binding, '{}' in expression", rhs_infer,
				    union_type->str());
			}

			if(not union_type->hasCaseNamed(this->variant_name.name))
			{
				return ErrMsg(m_location, "union type '{}' does not have a variant named '{}'", union_type->str(),
				    this->variant_name.name);
			}

			const auto variant_idx = union_type->getCaseIndex(this->variant_name.name);
			const auto variant_type = union_type->getCaseAtIndex(variant_idx);

			const auto is_any_mutable = std::any_of(this->bindings.begin(), this->bindings.end(), [](const auto& b) {
				return b.mut;
			});

			// FIXME: this manual bullshit is fragile as fuck
			std::unique_ptr<cst::VariableDefn> rhs_defn {};
			std::vector<std::unique_ptr<cst::VariableDefn>> binding_defns {};
			{
				auto variant_ptr_type = variant_type->pointerTo(/* mut: */ is_any_mutable);

				// make a temporary variable corresponding to the casted union member
				auto tmp_union_decl_ = cst::Declaration(m_location, ts->current(), ts->getTemporaryName(),
				    variant_ptr_type, is_any_mutable);

				auto tmp_union_decl = TRY(ts->current()->declare(std::move(tmp_union_decl_)));

				rhs_defn = std::make_unique<cst::VariableDefn>(m_location, tmp_union_decl, /* global: */ false, nullptr,
				    is_any_mutable);
				tmp_union_decl->define(rhs_defn.get());
			}

			for(auto& binding : this->bindings)
			{
				if(not binding.field_name.empty())
				{
					if(not variant_type->hasFieldNamed(binding.field_name))
					{
						return ErrMsg(binding.loc, "variant '{}' has no field named '{}'", this->variant_name.name,
						    binding.field_name);
					}

					// make an ident corresponding to the var we just declared
					const auto& union_name = rhs_defn->declaration->name;

					auto union_var = std::make_unique<cst::Ident>(m_location, rhs_defn->declaration->type,
					    QualifiedId::named(union_name), rhs_defn->declaration);
					union_var->resolved_decl = rhs_defn->declaration;

					auto var_type = variant_type->getFieldNamed(binding.field_name);
					const auto field_type = var_type;

					auto decl_ = cst::Declaration(m_location, ts->current(), binding.binding_name,
					    binding.reference ? var_type->pointerTo(binding.mut) : var_type, binding.mut);
					auto decl = TRY(ts->current()->declare(std::move(decl_)));

					auto deref_op = std::make_unique<cst::DereferenceOp>(m_location, var_type, std::move(union_var));
					auto dotop = std::make_unique<cst::DotOp>(binding.loc, field_type, /* optional: */ false,
					    variant_type, std::move(deref_op), binding.field_name);

					std::unique_ptr<cst::Expr> binding_expr = std::move(dotop);
					if(binding.reference)
					{
						binding_expr = std::make_unique<cst::AddressOfOp>(m_location, var_type->pointerTo(binding.mut),
						    binding.mut, std::move(binding_expr));
					}

					auto var_defn = std::make_unique<cst::VariableDefn>(m_location, decl, /* global: */ false,
					    std::move(binding_expr), binding.mut);

					decl->define(var_defn.get());

					binding_defns.push_back(std::move(var_defn));
				}
				else
				{
					// TODO
					return ErrMsg(m_location, "not supported sorry");
				}
			}


			if_stmt = std::make_unique<cst::IfLetUnionStmt>(m_location, union_type, variant_idx, std::move(rhs_defn),
			    std::move(binding_defns), std::move(rhs));

			if_stmt->true_case = TRY(this->true_case->typecheck(ts)).take_block();
			// the new scope is popped here
		}

		if(this->else_case)
			if_stmt->else_case = TRY(this->else_case->typecheck(ts)).take_block();

		return TCResult::ofVoid(std::move(if_stmt));
	}
}
