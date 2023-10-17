// subscript_op.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult2> SubscriptOp::typecheck_impl2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lhs = TRY(this->array->typecheck2(ts));

		// TODO: multiple subscript indices for overloaded operators
		auto rhs = TRY(this->indices[0]->typecheck2(ts));

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
				return TCResult2::ofMutableLValue<cst::SubscriptOp>(m_location, ltype->arrayElement(),
				    std::move(lhs).take_expr(), std::move(new_indices));
			}
			else
			{
				return TCResult2::ofImmutableLValue<cst::SubscriptOp>(m_location, ltype->arrayElement(),
				    std::move(lhs).take_expr(), std::move(new_indices));
			}
		}
		else
		{
			return TCResult2::ofRValue<cst::SubscriptOp>(m_location, ltype->arrayElement(), std::move(lhs).take_expr(),
			    std::move(new_indices));
		}
	}
}
