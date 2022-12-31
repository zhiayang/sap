// tree_manip.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"       // for Style
#include "sap/font_family.h" // for FontStyle, FontStyle::Bold, FontStyl...

#include "interp/tree.h"        // for Text
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/builtin.h"     // for bold1, bold_italic1, italic1
#include "interp/basedefs.h"    // for InlineObject
#include "interp/eval_result.h" // for EvalResult

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


	static ErrorOr<EvalResult> apply_style1(Interpreter* cs, std::vector<Value>& args, const Style* style)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);

		auto& value = args[0];

		if(value.isTreeInlineObj())
		{
			auto tio = value.getTreeInlineObj();
			auto new_style = style->extend(tio->style());
			value.getTreeInlineObj()->setStyle(new_style);

			return EvalResult::of_value(std::move(value));
		}
		else
		{
			auto word = std::make_unique<tree::Text>(value.toString());
			word->setStyle(style);

			return EvalResult::of_value(Value::treeInlineObject(std::move(word)));
		}
	}


	ErrorOr<EvalResult> bold1(Interpreter* cs, std::vector<Value>& args)
	{
		return apply_style1(cs, args, &g_bold_style);
	}

	ErrorOr<EvalResult> italic1(Interpreter* cs, std::vector<Value>& args)
	{
		return apply_style1(cs, args, &g_italic_style);
	}

	ErrorOr<EvalResult> bold_italic1(Interpreter* cs, std::vector<Value>& args)
	{
		return apply_style1(cs, args, &g_bold_italic_style);
	}
}
