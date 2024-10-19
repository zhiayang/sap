// compare_op.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for is_one_of

#include "tree/base.h"

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
{
	const char* op_to_string(ComparisonOp::Op op)
	{
		switch(op)
		{
			case ComparisonOp::Op::LT: return "<";
			case ComparisonOp::Op::GT: return ">";
			case ComparisonOp::Op::LE: return "<=";
			case ComparisonOp::Op::GE: return ">=";
			case ComparisonOp::Op::EQ: return "==";
			case ComparisonOp::Op::NE: return "!=";
		}
		util::unreachable();
	}

	static cst::ComparisonOp::Op ast_op_to_cst_op(ComparisonOp::Op op)
	{
		switch(op)
		{
			case ComparisonOp::Op::LT: return cst::ComparisonOp::Op::LT;
			case ComparisonOp::Op::GT: return cst::ComparisonOp::Op::GT;
			case ComparisonOp::Op::LE: return cst::ComparisonOp::Op::LE;
			case ComparisonOp::Op::GE: return cst::ComparisonOp::Op::GE;
			case ComparisonOp::Op::EQ: return cst::ComparisonOp::Op::EQ;
			case ComparisonOp::Op::NE: return cst::ComparisonOp::Op::NE;
		}
		util::unreachable();
	}

	static bool can_compare(ComparisonOp::Op op, const Type* lhs, const Type* rhs)
	{
		if(lhs->isArray() && rhs->isArray())
			return can_compare(op, lhs->arrayElement(), rhs->arrayElement());

		if(util::is_one_of(op, ComparisonOp::Op::EQ, ComparisonOp::Op::NE))
		{
			return (lhs == rhs)                            //
			    || (lhs->isPointer() && rhs->isNullPtr())  //
			    || (lhs->isNullPtr() && rhs->isPointer())  //
			    || (lhs->isOptional() && rhs->isNullPtr()) //
			    || (lhs->isNullPtr() && rhs->isOptional());
		}

		// if we are doing ordering, make sure that the enum is integral (if it's an enum)
		if((lhs->isEnum() && not lhs->toEnum()->elementType()->isInteger())
		    || (rhs->isEnum() && not rhs->toEnum()->elementType()->isInteger()))
			return false;

		return lhs == rhs                      //
		    && (not lhs->isTreeInlineObj())    //
		    && (not lhs->isTreeBlockObj())     //
		    && (not lhs->isLayoutObject())     //
		    && (not lhs->isTreeInlineObjRef()) //
		    && (not lhs->isTreeBlockObjRef())  //
		    && (not lhs->isLayoutObjectRef())  //
		    && (not lhs->isFunction());
	}

	ErrorOr<TCResult> ComparisonOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(not this->rest.empty());

		auto lhs = TRY(this->first->typecheck(ts));

		auto op = this->rest[0].first;
		auto rhs = TRY(this->rest[0].second->typecheck(ts));

		auto ltype = lhs.type();
		auto rtype = rhs.type();

		auto new_first = std::move(lhs).take_expr();

		std::vector<std::pair<cst::ComparisonOp::Op, std::unique_ptr<cst::Expr>>> new_rest {};
		new_rest.emplace_back(ast_op_to_cst_op(op), std::move(rhs).take_expr());

		for(size_t i = 0;;)
		{
			if(not can_compare(op, ltype, rtype))
			{
				return ErrMsg(ts, "types '{}' and '{}' are not comparable with operator '{}'", ltype, rtype,
				    op_to_string(op));
			}

			if(i + 1 == this->rest.size())
				break;

			i += 1;
			ltype = rtype;
			op = this->rest[i].first;

			auto tmp = TRY(this->rest[i].second->typecheck(ts));
			rtype = tmp.type();

			new_rest.emplace_back(ast_op_to_cst_op(op), std::move(tmp).take_expr());
		}

		return TCResult::ofRValue<cst::ComparisonOp>(m_location, std::move(new_first), std::move(new_rest));
	}
}
