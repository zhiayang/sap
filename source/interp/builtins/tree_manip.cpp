// tree_manip.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"       // for Style
#include "sap/font_family.h" // for FontStyle, FontStyle::Bold, FontStyl...

#include "interp/tree.h"          // for Text
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


	template <typename T>
	static std::optional<T> get_field(Value& str, zst::str_view field)
	{
		auto& fields = str.getStructFields();
		auto idx = str.type()->toStruct()->getFieldIndex(field);

		auto& f = fields[idx];
		if(not f.haveOptionalValue())
			return std::nullopt;

		return (*f.getOptional())->get<T>();
	}

	ErrorOr<EvalResult> apply_style(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		assert(args[0].type() == BStyle::type);

		auto get_scalar = [&args](zst::str_view field) -> std::optional<sap::Length> {
			auto f = get_field<double>(args[0], field);
			if(f.has_value())
				return sap::Length(*f);

			return std::nullopt;
		};

		auto style = Style();
		style.set_font_size(get_scalar("font_size"));
		style.set_line_spacing(get_field<double>(args[0], "line_spacing"));

		return do_apply_style(ev, args[1], &style);
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




	ErrorOr<EvalResult> load_image(Evaluator* ev, std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 3);

		auto img_path = args[0].getUtf8String();
		zpr::println("loading image '{}'", img_path);

		auto img_width = sap::Length(args[1].getFloating());
		std::optional<sap::Length> img_height {};

		if(auto tmp = std::move(args[2]).takeOptional(); tmp.has_value())
			img_height = sap::Length(tmp->getFloating());

		auto img_obj = tree::Image::fromImageFile(img_path, img_width, img_height);
		return EvalResult::ofValue(Value::treeBlockObject(std::move(img_obj)));
	}


	ErrorOr<EvalResult> centred_block(Evaluator* ev, std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);

		auto container = std::make_unique<tree::CentredContainer>(std::move(args[0]).takeTreeBlockObj());
		return EvalResult::ofValue(Value::treeBlockObject(std::move(container)));
	}
}
