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

	static ErrorOr<EvalResult> do_apply_style(Evaluator* ev, Value& value, const Style* style)
	{
		if(value.isTreeInlineObj())
		{
			auto tios = std::move(value).takeTreeInlineObj();
			for(auto& obj : tios)
				obj->setStyle(style->extendWith(obj->style()));

			return EvalResult::ofValue(Value::treeInlineObject(std::move(tios)));
		}
		else
		{
			auto word = std::make_unique<tree::Text>(value.toString());
			word->setStyle(style);

			std::vector<std::unique_ptr<tree::InlineObject>> tmp;
			tmp.push_back(std::move(word));

			return EvalResult::ofValue(Value::treeInlineObject(std::move(tmp)));
		}
	}


	ErrorOr<EvalResult> apply_style(Evaluator* ev, std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 2);


		return do_apply_style(ev, args[0], &g_bold_style);
	}


	ErrorOr<EvalResult> bold1(Evaluator* ev, std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);
		return do_apply_style(ev, args[0], &g_bold_style);
	}

	ErrorOr<EvalResult> italic1(Evaluator* ev, std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);
		return do_apply_style(ev, args[0], &g_italic_style);
	}

	ErrorOr<EvalResult> bold_italic1(Evaluator* ev, std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);
		return do_apply_style(ev, args[0], &g_bold_italic_style);
	}
}
