// tree_manip.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"       // for Style
#include "sap/font_family.h" // for FontStyle, FontStyle::Bold, FontStyl...

#include "tree/base.h"
#include "tree/image.h"
#include "tree/paragraph.h"
#include "tree/container.h"

#include "interp/value.h"         // for Value
#include "interp/interp.h"        // for Interpreter
#include "interp/basedefs.h"      // for InlineObject
#include "interp/eval_result.h"   // for EvalResult
#include "interp/builtin_fns.h"   // for bold1, bold_italic1, italic1
#include "interp/builtin_types.h" //

#include "layout/base.h"

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
				obj->setStyle(obj->style()->extendWith(style));

			return EvalResult::ofValue(Value::treeInlineObject(std::move(tios)));
		}
		else if(value.isTreeBlockObj())
		{
			auto tbo = std::move(value).takeTreeBlockObj();
			tbo->setStyle(style);

			return EvalResult::ofValue(Value::treeBlockObject(std::move(tbo)));
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
		return EvalResult::ofValue(TRY(BS_Style::make(ev, ev->currentStyle())));
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
		return EvalResult::ofValue(TRY(BS_Style::make(ev, ev->popStyle())));
	}








	ErrorOr<EvalResult> load_image(Evaluator* ev, std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 3);

		auto img_path = args[0].getUtf8String();
		zpr::println("loading image '{}'", img_path);

		auto style = ev->currentStyle();

		auto img_width = args[1].getLength().resolve(style->font(), style->font_size(), style->root_font_size());

		std::optional<sap::Length> img_height {};
		if(auto tmp = std::move(args[2]).takeOptional(); tmp.has_value())
			img_height = tmp->getLength().resolve(style->font(), style->font_size(), style->root_font_size());

		auto img_obj = tree::Image::fromImageFile(img_path, img_width, img_height);
		return EvalResult::ofValue(Value::treeBlockObject(std::move(img_obj)));
	}




	ErrorOr<EvalResult> make_text(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		std::vector<std::unique_ptr<tree::InlineObject>> inline_objs {};

		auto strings = std::move(args[0]).takeArray();
		for(size_t i = 0; i < strings.size(); i++)
		{
			if(i != 0)
				inline_objs.push_back(std::make_unique<tree::Separator>(tree::Separator::SPACE));

			inline_objs.push_back(std::make_unique<tree::Text>(strings[i].getUtf32String()));
		}

		return EvalResult::ofValue(Value::treeInlineObject(std::move(inline_objs)));
	}



	ErrorOr<EvalResult> make_paragraph(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		auto para = std::make_unique<tree::Paragraph>();

		auto objs = std::move(args[0]).takeArray();
		for(size_t i = 0; i < objs.size(); i++)
			para->addObjects(std::move(objs[i]).takeTreeInlineObj());

		return EvalResult::ofValue(Value::treeBlockObject(std::move(para)));
	}

	ErrorOr<EvalResult> make_line(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		auto para = std::make_unique<tree::Paragraph>();

		auto objs = std::move(args[0]).takeArray();
		for(size_t i = 0; i < objs.size(); i++)
			para->addObjects(std::move(objs[i]).takeTreeInlineObj());

		para->setSingleLineMode(true);
		return EvalResult::ofValue(Value::treeBlockObject(std::move(para)));
	}




	ErrorOr<EvalResult> current_layout_position(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 0);

		return EvalResult::ofValue(TRY(BS_Position::make(ev, ev->getBlockContext().parent_pos)));
	}
}
