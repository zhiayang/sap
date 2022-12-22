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

			auto word = std::make_unique<tree::Word>(value.toString());
			auto style = &Style::combine(word->style(), nullptr)->clone()->set_font_style(FontStyle::Bold);
			word->setStyle(style);

			return Ok(Value::treeInlineObject(std::move(word)));
		}

		return ErrFmt("aff");
		// return tio_expr;
	}
}
