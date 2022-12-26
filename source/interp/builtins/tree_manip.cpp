// tree_manip.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"   // for Style
#include "sap/fontset.h" // for FontStyle, FontStyle::Bold, FontStyle::...

#include "interp/tree.h"     // for Text
#include "interp/type.h"     // for Type
#include "interp/state.h"    // for DefnTree, Interpreter
#include "interp/value.h"    // for Value
#include "interp/interp.h"   // for makeParamList, BuiltinFunctionDefn, Def...
#include "interp/builtin.h"  // for defineBuiltins
#include "interp/basedefs.h" // for InlineObject

namespace sap::interp::builtin
{
	static const Style g_bold_style = []() {
		Style style {};
		style.set_font_style(FontStyle::Bold);
		return style;
	}();

	static const Style g_italic_style = []() {
		Style style {};
		style.set_font_style(FontStyle::Italic);
		return style;
	}();

	static const Style g_bold_italic_style = []() {
		Style style {};
		style.set_font_style(FontStyle::BoldItalic);
		return style;
	}();


	static ErrorOr<std::optional<Value>> apply_style1(Interpreter* cs, std::vector<Value>& args, const Style& style)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);

		auto& value = args[0];

		if(value.isTreeInlineObj())
		{
			auto tio = value.getTreeInlineObj();
			auto new_style = Style::combine(tio->style(), &style);
			value.getTreeInlineObj()->setStyle(new_style);

			return Ok(std::move(value));
		}
		else
		{
			auto word = std::make_unique<tree::Text>(value.toString());
			word->setStyle(&style);

			return Ok(Value::treeInlineObject(std::move(word)));
		}
	}


	ErrorOr<std::optional<Value>> bold1(Interpreter* cs, std::vector<Value>& args)
	{
		return apply_style1(cs, args, g_bold_style);
	}

	ErrorOr<std::optional<Value>> italic1(Interpreter* cs, std::vector<Value>& args)
	{
		return apply_style1(cs, args, g_italic_style);
	}

	ErrorOr<std::optional<Value>> bold_italic1(Interpreter* cs, std::vector<Value>& args)
	{
		return apply_style1(cs, args, g_bold_italic_style);
	}
}
