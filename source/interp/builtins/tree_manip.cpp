// tree_manip.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/tree.h"
#include "interp/state.h"
#include "interp/builtin.h"

namespace sap::interp::builtin
{
	ErrorOr<std::optional<Value>> bold1(Interpreter* cs, const std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);

		auto& value = args[0];

		if(value.isTreeInlineObj())
		{
			zpr::println("xxx");
		}
		else
		{
			zpr::println("aoeu");
			return Ok(Value::treeInlineObject(std::make_unique<tree::Word>(value.toString())));
		}

		return ErrFmt("aff");
		// return tio_expr;
	}
}
