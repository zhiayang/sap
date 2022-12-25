// tree_manip.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <memory> // for make_unique, unique_ptr

#include "defs.h" // for ErrorOr, Ok

#include "sap/style.h"   // for Style
#include "sap/fontset.h" // for FontStyle, FontStyle::Bold

#include "interp/tree.h"    // for Text, InlineObject
#include "interp/state.h"   // for Interpreter
#include "interp/value.h"   // for Value
#include "interp/builtin.h" // for bold1

namespace sap::interp::builtin
{
	static const Style bold_style = []() {
		Style style;
		style.set_font_style(FontStyle::Bold);
		return style;
	}();

	ErrorOr<std::optional<Value>> bold1(Interpreter* cs, std::vector<Value>& args)
	{

		// TODO: maybe don't assert?
		assert(args.size() == 1);

		auto& value = args[0];

		if(value.isTreeInlineObj())
		{
			auto tio = value.getTreeInlineObj();
			auto style = Style::combine(tio->style(), &bold_style);
			value.getTreeInlineObj()->setStyle(style);

			return Ok(std::move(value));
		}
		else
		{
			auto word = std::make_unique<tree::Text>(value.toString());
			word->setStyle(&bold_style);

			return Ok(Value::treeInlineObject(std::move(word)));
		}
	}
}
