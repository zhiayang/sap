// dotop.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> DotOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto lhs_res = TRY(this->lhs->typecheck(ts));
		auto ltype = lhs_res.type();

		const StructType* struct_type = nullptr;
		if(this->is_optional)
		{
			if(not ltype->isOptional() && not ltype->isPointer())
				return ErrMsg(ts, "invalid use of '?.' operator on a non-pointer, non-optional type '{}'", ltype);

			auto lelm_type = ltype->isPointer() ? ltype->pointerElement() : ltype->optionalElement();
			if(not lelm_type->isStruct())
				return ErrMsg(ts, "invalid use of '?.' operator on a non-struct type '{}'", lelm_type);

			struct_type = lelm_type->toStruct();
		}
		else
		{
			if(not ltype->isStruct())
				return ErrMsg(ts, "invalid use of '.' operator on a non-struct type '{}'", ltype);

			struct_type = ltype->toStruct();
		}

		assert(struct_type != nullptr);
		if(not struct_type->hasFieldNamed(this->rhs))
			return ErrMsg(ts, "type '{}' has no field named '{}'", ltype, this->rhs);

		auto field_type = struct_type->getFieldNamed(this->rhs);
		const Type* result_type = [&]() {
			if(this->is_optional)
			{
				return ltype->isOptional() //
				         ? (const Type*) field_type->optionalOf()
				         : (const Type*) field_type->pointerTo(ltype->isMutablePointer());
			}
			else
			{
				return field_type;
			}
		}();

		bool is_lvalue = lhs_res.isLValue();
		bool is_mutable = lhs_res.isMutable();

		auto dot_op = std::make_unique<cst::DotOp>(m_location, result_type, this->is_optional, struct_type,
		    std::move(lhs_res).take_expr(), this->rhs);

		if(this->is_optional)
		{
			// if we're optional, make an rvalue always.
			// this means we can't do a?.b = ... , but that's fine probably.
			return TCResult::ofRValue(std::move(dot_op));
		}
		else
		{
			// otherwise, we copy the lvalue-ness and mutability of the lhs.
			if(is_lvalue)
				return TCResult::ofLValue(std::move(dot_op), is_mutable);
			else
				return TCResult::ofRValue(std::move(dot_op));
		}
	}
}
