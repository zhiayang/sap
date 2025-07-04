// styling.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "tree/container.h"

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

	static ErrorOr<EvalResult> do_apply_style(Evaluator* ev, Value& value, const Style& style)
	{
		if(value.isTreeInlineObj())
		{
			auto& tios = value.getTreeInlineObj();
			for(auto& obj : tios.objects())
				obj->setStyle(obj->style().extendWith(style));

			return EvalResult::ofValue(std::move(value));
		}
		else if(value.isTreeBlockObj())
		{
			auto& tbo = value.getTreeBlockObj();
			const_cast<tree::BlockObject&>(tbo).setStyle(tbo.style().extendWith(style));

			return EvalResult::ofValue(std::move(value));
		}
		else
		{
			auto word = TRY(ev->convertValueToText(std::move(value)));
			assert(word->isSpan());

			for(auto& w : word->castToSpan()->objects())
				w->setStyle(style);

			return EvalResult::ofValue(Value::treeInlineObject(std::move(word)));
		}
	}

	ErrorOr<EvalResult> set_style_tio(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);

		const auto style = TRY(BS_Style::unmake(ev, args[1]));
		auto& tios = args[0].getMutablePointer()->getTreeInlineObj();

		for(auto& obj : tios.objects())
			obj->setStyle(obj->style().extendWith(style));

		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> set_style_tbo(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);

		const auto style = TRY(BS_Style::unmake(ev, args[1]));
		auto& tbo = args[0].getMutablePointer()->getTreeBlockObj();

		const_cast<tree::BlockObject&>(tbo).setStyle(tbo.style().extendWith(style));
		return EvalResult::ofVoid();
	}


	ErrorOr<EvalResult> apply_style_tio(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);

		if(args[0].type() == BS_Style::type)
			return do_apply_style(ev, args[1], TRY(BS_Style::unmake(ev, args[0])));
		else
			return do_apply_style(ev, args[0], TRY(BS_Style::unmake(ev, args[1])));
	}

	ErrorOr<EvalResult> apply_style_tbo(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);

		if(args[0].type() == BS_Style::type)
			return do_apply_style(ev, args[1], TRY(BS_Style::unmake(ev, args[0])));
		else
			return do_apply_style(ev, args[0], TRY(BS_Style::unmake(ev, args[1])));
	}


	ErrorOr<EvalResult> bold1(Evaluator* ev, std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);
		return do_apply_style(ev, args[0], g_bold_style);
	}

	ErrorOr<EvalResult> italic1(Evaluator* ev, std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);
		return do_apply_style(ev, args[0], g_italic_style);
	}

	ErrorOr<EvalResult> bold_italic1(Evaluator* ev, std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);
		return do_apply_style(ev, args[0], g_bold_italic_style);
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

	ErrorOr<EvalResult> set_border_style(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);

		BorderStyle bs {};
		Value* obj;

		if(args[0].type() == BS_BorderStyle::type)
			(bs = BS_BorderStyle::unmake(ev, args[0])), (obj = &args[1]);
		else
			(bs = BS_BorderStyle::unmake(ev, args[1])), (obj = &args[0]);

		auto tbo = std::move(*obj).takeTreeBlockObj();
		auto* container = tbo->castToContainer();
		if(not container)
			return ErrMsg(ev, "border styles can only be set on containers (for now?)");

		container->setBorderStyle(std::move(bs));
		return EvalResult::ofValue(Value::treeBlockObject(std::move(tbo)));
	}
}
