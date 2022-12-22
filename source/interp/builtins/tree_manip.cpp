// tree_manip.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/tree.h"
#include "interp/state.h"
#include "interp/builtin.h"

namespace sap::interp::builtin
{
	ErrorOr<std::optional<Value>> bold1(Interpreter* cs, std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);

		auto& value = args[0];

		if(value.isTreeInlineObj())
		{
			auto tio = value.getTreeInlineObj();
			auto style = &Style::combine(tio->style(), nullptr)->clone()->set_font_style(FontStyle::Bold);
			value.getTreeInlineObj()->setStyle(style);

			return Ok(std::move(value));
		}
		else
		{
			auto word = std::make_unique<tree::Word>(value.toString());
			auto style = &defaultStyle().clone()->set_font_style(FontStyle::Bold);
			word->setStyle(style);

			return Ok(Value::treeInlineObject(std::move(word)));
		}
	}
}
