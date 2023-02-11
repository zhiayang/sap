// tree_manip.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"       // for Style
#include "sap/font_family.h" // for FontStyle, FontStyle::Bold, FontStyl...

#include "tree/base.h"
#include "tree/image.h"
#include "tree/wrappers.h"
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








	ErrorOr<EvalResult> load_image(Evaluator* ev, std::vector<Value>& args)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 3);

		auto img_path = args[0].getUtf8String();

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

	static ErrorOr<EvalResult> make_box(Evaluator* ev, std::vector<Value>& args, tree::Container::Direction direction)
	{
		assert(args.size() == 1);

		auto box = std::make_unique<tree::Container>(direction);

		auto objs = std::move(args[0]).takeArray();
		for(size_t i = 0; i < objs.size(); i++)
		{
			auto tbo = std::move(objs[i]).takeTreeBlockObj();
			assert(tbo != nullptr);

			box->contents().push_back(std::move(tbo));
		}

		return EvalResult::ofValue(Value::treeBlockObject(std::move(box)));
	}

	ErrorOr<EvalResult> make_hbox(Evaluator* ev, std::vector<Value>& args)
	{
		return make_box(ev, args, tree::Container::Direction::Horizontal);
	}

	ErrorOr<EvalResult> make_vbox(Evaluator* ev, std::vector<Value>& args)
	{
		return make_box(ev, args, tree::Container::Direction::Vertical);
	}

	ErrorOr<EvalResult> make_zbox(Evaluator* ev, std::vector<Value>& args)
	{
		return make_box(ev, args, tree::Container::Direction::None);
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

		auto line = std::make_unique<tree::WrappedLine>();

		auto objs = std::move(args[0]).takeArray();
		for(size_t i = 0; i < objs.size(); i++)
			line->addObjects(std::move(objs[i]).takeTreeInlineObj());

		return EvalResult::ofValue(Value::treeBlockObject(std::move(line)));
	}

	ErrorOr<EvalResult> get_layout_object(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);
		assert(args[0].isPointer());

		auto& value = *args[0].getPointer();

		assert(value.type()->isTreeBlockObj() || value.type()->isTreeBlockObjRef());


		// of course, this assumes that the order of enumerators in ProcessingPhase
		// is according to the order that the phases happen.
		if(ev->interpreter()->currentPhase() <= ProcessingPhase::Layout)
			return ErrMsg(ev, "`layout_object()` can only be called after the layout phase");

		auto layout_obj = value.isTreeBlockObj()
		                    ? value.getTreeBlockObj().getGeneratedLayoutObject()
		                    : value.getTreeBlockObjectRef()->getGeneratedLayoutObject();

		auto ref_type = Type::makeLayoutObjectRef();

		std::optional<Value> tmp {};
		if(layout_obj.has_value())
			tmp = Value::layoutObjectRef(*layout_obj);

		return EvalResult::ofValue(Value::optional(ref_type, std::move(tmp)));
	}
}
