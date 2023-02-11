// styling.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/paragraph.h"

#include "interp/interp.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

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
			auto& tios = value.getTreeInlineObj();
			for(auto& obj : tios)
				obj->setStyle(obj->style()->extendWith(style));

			return EvalResult::ofValue(std::move(value));
		}
		else if(value.isTreeBlockObj())
		{
			auto& tbo = value.getTreeBlockObj();
			const_cast<tree::BlockObject&>(tbo).setStyle(style);

			return EvalResult::ofValue(std::move(value));
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

	ErrorOr<EvalResult> apply_style_tio(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		assert(args[0].type() == BS_Style::type);

		return do_apply_style(ev, args[1], TRY(BS_Style::unmake(ev, args[0])));
	}

	ErrorOr<EvalResult> apply_style_tbo(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		assert(args[0].type() == BS_Style::type);

		return do_apply_style(ev, args[1], TRY(BS_Style::unmake(ev, args[0])));
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


	ErrorOr<EvalResult> current_style(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 0);
		return EvalResult::ofValue(BS_Style::make(ev, ev->currentStyle()));
	}

	ErrorOr<EvalResult> push_style(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		ev->pushStyle(TRY(BS_Style::unmake(ev, args[0])));
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> pop_style(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 0);
		return EvalResult::ofValue(BS_Style::make(ev, ev->popStyle()));
	}
}
