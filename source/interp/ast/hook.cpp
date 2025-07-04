// hook.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> HookBlock::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		if(ts->isCurrentlyInFunction())
			return ErrMsg(ts, "hook blocks can only appear the top level");

		auto block = TRY(this->body->typecheck(ts, infer, keep_lvalue)).take<cst::Block>();
		ts->interpreter()->addHookBlock(this->phase, block.get());

		return TCResult::ofVoid<cst::HookBlock>(m_location, std::move(block));
	}
}
