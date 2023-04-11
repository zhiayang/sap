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
#include "layout/word.h"

namespace sap::interp::builtin
{
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

		auto img_obj = TRY(tree::Image::fromImageFile(ev->loc(), img_path, img_width, img_height));
		return EvalResult::ofValue(Value::treeBlockObject(std::move(img_obj)));
	}




	ErrorOr<EvalResult> make_text(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		auto span = zst::make_shared<tree::InlineSpan>();

		auto strings = std::move(args[0]).takeArray();
		for(size_t i = 0; i < strings.size(); i++)
		{
			if(i != 0)
				span->addObject(zst::make_shared<tree::Separator>(tree::Separator::SPACE));

			span->addObject(zst::make_shared<tree::Text>(strings[i].getUtf32String()));
		}

		return EvalResult::ofValue(Value::treeInlineObject(std::move(span)));
	}

	static ErrorOr<EvalResult> make_box(Evaluator* ev,
	    Value& arr,
	    tree::Container::Direction direction,
	    bool glued = false)
	{
		auto box = zst::make_shared<tree::Container>(direction, glued);

		auto objs = std::move(arr).takeArray();
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
		assert(args.size() == 1);
		return make_box(ev, args[0], tree::Container::Direction::Horizontal);
	}

	ErrorOr<EvalResult> make_vbox(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		return make_box(ev, args[0], tree::Container::Direction::Vertical, /* glued: */ args[1].getBool());
	}

	ErrorOr<EvalResult> make_zbox(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);
		return make_box(ev, args[0], tree::Container::Direction::None);
	}


	ErrorOr<EvalResult> make_span(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		auto span = zst::make_shared<tree::InlineSpan>();

		auto objs = std::move(args[0]).takeArray();
		for(size_t i = 0; i < objs.size(); i++)
			span->addObject(std::move(objs[i]).takeTreeInlineObj());

		return EvalResult::ofValue(Value::treeInlineObject(std::move(span)));
	}

	ErrorOr<EvalResult> make_paragraph(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		auto objs = std::move(args[0]).takeArray();
		std::vector<zst::SharedPtr<tree::InlineObject>> inlines {};

		for(size_t i = 0; i < objs.size(); i++)
			inlines.push_back(std::move(objs[i]).takeTreeInlineObj());

		auto para = zst::make_shared<tree::Paragraph>();

		inlines = TRY(tree::Paragraph::processWordSeparators(std::move(inlines)));
		para->addObjects(std::move(inlines));

		return EvalResult::ofValue(Value::treeBlockObject(std::move(para)));
	}

	ErrorOr<EvalResult> make_line(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		auto line = zst::make_shared<tree::WrappedLine>();

		auto objs = std::move(args[0]).takeArray();
		std::vector<zst::SharedPtr<tree::InlineObject>> inlines {};

		for(size_t i = 0; i < objs.size(); i++)
			inlines.push_back(std::move(objs[i]).takeTreeInlineObj());

		inlines = TRY(tree::Paragraph::processWordSeparators(std::move(inlines)));
		line->addObjects(std::move(inlines));

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




	ErrorOr<EvalResult> set_tbo_size(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		assert(args[0].isPointer());

		auto& value = *args[0].getPointer();
		assert(value.type()->isTreeBlockObj() || value.type()->isTreeBlockObjRef());

		if(ev->interpreter()->currentPhase() != ProcessingPhase::Layout)
			return ErrMsg(ev, "`set_size()` can only be called during `@layout`");

		auto size = BS_Size2d::unmake(ev, args[1]).resolve(ev->currentStyle());
		auto tbo = value.isTreeBlockObj() ? &value.getTreeBlockObj() : value.getTreeBlockObjectRef();

		const_cast<tree::BlockObject*>(tbo)->overrideLayoutWidth(size.x());
		const_cast<tree::BlockObject*>(tbo)->overrideLayoutHeight(size.y());
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> set_tbo_width(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		assert(args[0].isPointer());

		auto& value = *args[0].getPointer();
		assert(value.type()->isTreeBlockObj() || value.type()->isTreeBlockObjRef());

		if(ev->interpreter()->currentPhase() != ProcessingPhase::Layout)
			return ErrMsg(ev, "`set_width()` can only be called during `@layout`");

		auto size = args[1].getLength().resolve(ev->currentStyle());
		auto tbo = value.isTreeBlockObj() ? &value.getTreeBlockObj() : value.getTreeBlockObjectRef();

		const_cast<tree::BlockObject*>(tbo)->overrideLayoutWidth(size);
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> set_tbo_height(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		assert(args[0].isPointer());

		auto& value = *args[0].getPointer();
		assert(value.type()->isTreeBlockObj() || value.type()->isTreeBlockObjRef());

		if(ev->interpreter()->currentPhase() != ProcessingPhase::Layout)
			return ErrMsg(ev, "`set_height()` can only be called during `@layout`");

		auto size = args[1].getLength().resolve(ev->currentStyle());
		auto tbo = value.isTreeBlockObj() ? &value.getTreeBlockObj() : value.getTreeBlockObjectRef();

		const_cast<tree::BlockObject*>(tbo)->overrideLayoutHeight(size);
		return EvalResult::ofVoid();
	}



	ErrorOr<EvalResult> set_tio_width(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);

		Value* tio = nullptr;
		if(args[0].isPointer())
			tio = args[0].getMutablePointer();
		else
			tio = &args[0];

		assert(tio->type()->isTreeInlineObj());

		if(ev->interpreter()->currentPhase() != ProcessingPhase::Layout)
			return ErrMsg(ev, "`set_width()` can only be called during `@layout`");

		auto width = args[1].getLength().resolve(ev->currentStyle());
		const_cast<tree::InlineSpan&>(tio->getTreeInlineObj()).overrideWidth(width);

		if(args[0].isPointer())
		{
			return EvalResult::ofValue(Value::mutablePointer(Type::makeTreeInlineObj()->pointerTo(),
			    args[0].getMutablePointer()));
		}
		else
		{
			return EvalResult::ofValue(std::move(args[0]));
		}
	}



	static void apply_raise(std::vector<zst::SharedPtr<tree::InlineObject>>& objs, Length raise)
	{
		for(auto& obj : objs)
		{
			if(auto span = dynamic_cast<tree::InlineSpan*>(obj.get()))
			{
				span->addRaiseHeight(raise);
				apply_raise(span->objects(), raise);
			}
			else
			{
				assert(dynamic_cast<tree::Text*>(obj.get()) || dynamic_cast<tree::Separator*>(obj.get()));
				obj->addRaiseHeight(raise);
			}
		}
	}

	ErrorOr<EvalResult> raise_tio(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);

		Value* tio = nullptr;
		if(args[0].isPointer())
			tio = args[0].getMutablePointer();
		else
			tio = &args[0];

		assert(tio->type()->isTreeInlineObj());

		if(ev->interpreter()->currentPhase() != ProcessingPhase::Layout)
			return ErrMsg(ev, "`raise()` can only be called during `@layout`");

		auto raise = args[1].getLength().resolve(ev->currentStyle());
		apply_raise(const_cast<tree::InlineSpan&>(tio->getTreeInlineObj()).objects(), raise);

		if(args[0].isPointer())
		{
			return EvalResult::ofValue(Value::mutablePointer(Type::makeTreeInlineObj()->pointerTo(),
			    args[0].getMutablePointer()));
		}
		else
		{
			return EvalResult::ofValue(std::move(args[0]));
		}
	}



	template <typename T>
	static ErrorOr<EvalResult> set_link_annotation(Evaluator* ev, T* obj, Value& arg)
	{
		if(ev->interpreter()->currentPhase() >= ProcessingPhase::Render)
			return ErrMsg(ev, "`link_to()` can only be called before `@render`");

		if(arg.type() == BS_AbsPosition::type)
			obj->setLinkDestination(BS_AbsPosition::unmake(ev, arg));
		else if(arg.type()->isTreeBlockObjRef())
			obj->setLinkDestination(arg.getTreeBlockObjectRef());
		else if(arg.type()->isTreeInlineObjRef())
			obj->setLinkDestination(arg.getTreeInlineObjectRef());
		else
			return ErrMsg(ev, "unsupported link target '{}'", arg.type());

		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> set_lo_link_annotation(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);

		auto& value = *args[0].getPointer();
		assert(value.isLayoutObject() || value.isLayoutObjectRef());

		auto obj = const_cast<layout::LayoutObject*>(
		    args[0].isLayoutObject() //
		        ? &value.getLayoutObject()
		        : value.getLayoutObjectRef());

		return set_link_annotation(ev, obj, args[1]);
	}

	ErrorOr<EvalResult> set_tbo_link_annotation(Evaluator* ev, std::vector<Value>& args)
	{
		auto& value = *args[0].getPointer();
		assert(value.type()->isTreeBlockObj() || value.type()->isTreeBlockObjRef());

		auto tbo = const_cast<tree::BlockObject*>(
		    value.isTreeBlockObj() //
		        ? &value.getTreeBlockObj()
		        : value.getTreeBlockObjectRef());

		return set_link_annotation(ev, tbo, args[1]);
	}


	ErrorOr<EvalResult> set_tio_link_annotation(Evaluator* ev, std::vector<Value>& args)
	{
		auto& value = *args[0].getPointer();
		assert(value.type()->isTreeInlineObj() || value.type()->isTreeInlineObjRef());

		auto tio = const_cast<tree::InlineSpan*>(
		    value.isTreeInlineObj() //
		        ? &value.getTreeInlineObj()
		        : value.getTreeInlineObjectRef());

		TRY(set_link_annotation(ev, tio, args[1]));

		for(auto ls : tio->generatedLayoutSpans())
			TRY(set_link_annotation(ev, ls, args[1]));

		return EvalResult::ofVoid();
	}
}
