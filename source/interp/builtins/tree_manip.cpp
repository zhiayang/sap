// tree_manip.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/tree.h"
#include "interp/state.h"
#include "interp/builtin.h"

namespace sap::interp::builtin
{
	std::unique_ptr<Expr> bold1(Interpreter* cs, const std::vector<const Expr*>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);

		auto arg = args[0];
		auto tio_expr = std::make_unique<InlineTreeExpr>();

		if(arg->type->isNumber())
		{
			// tio_expr->object = std::make_unique<tree::Word>(std::to_string(arg));
		}
		else if(arg->type->isString())
		{
		}
		else if(arg->type->isTreeInlineObj())
		{
		}
		else
		{
			assert(false && "unreachable!");
			return nullptr;
		}

		return nullptr;
	}
}
