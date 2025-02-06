// subscript_op.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"
#include "interp/polymorph.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> SubscriptOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		// check whether this is a explicit generic-function instantiation
		// needs to fulfil these criteria:
		// 1. lhs is an identifier
		// 2. we can resolve the lhs identifier to a generic decl
		if(auto lhs_ident = dynamic_cast<const Ident*>(this->array.get()))
		{
			auto _x = ts->current()->lookup(lhs_ident->name);
			if(_x.is_err())
				goto not_generic_function;

			auto& decls = _x.unwrap();

			bool found_generic = false;
			for(auto& decl : decls)
			{
				if(decl->generic_func != nullptr)
					found_generic = true;
			}

			if(not found_generic)
				goto not_generic_function;

			// ok we have at least one generic function, try to do some generic stuff.
			return polymorph::tryInstantiateGenericFunction(ts, infer, lhs_ident->name, std::move(decls),
			    this->indices);
		}

	not_generic_function:
		auto lhs = TRY(this->array->typecheck(ts, /* infer: */ nullptr, keep_lvalue));

		// TODO: multiple subscript indices for overloaded operators
		auto rhs = TRY(this->indices[0].value->typecheck(ts));

		auto ltype = lhs.type();
		auto rtype = rhs.type();
		if(not ltype->isArray())
			return ErrMsg(ts, "invalid use of operator '[]' on non-array type '{}'", ltype);
		if(not rtype->isInteger())
			return ErrMsg(ts, "subscript index must be an integer");

		std::vector<std::unique_ptr<cst::Expr>> new_indices {};
		new_indices.push_back(std::move(rhs).take_expr());

		// FIXME: lots of code dupe
		if(lhs.isLValue())
		{
			if(lhs.isMutable())
			{
				return TCResult::ofMutableLValue<cst::SubscriptOp>(m_location, ltype->arrayElement(),
				    std::move(lhs).take_expr(), std::move(new_indices));
			}
			else
			{
				return TCResult::ofImmutableLValue<cst::SubscriptOp>(m_location, ltype->arrayElement(),
				    std::move(lhs).take_expr(), std::move(new_indices));
			}
		}
		else
		{
			return TCResult::ofRValue<cst::SubscriptOp>(m_location, ltype->arrayElement(), std::move(lhs).take_expr(),
			    std::move(new_indices));
		}
	}
}
