// positioning.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/spacer.h"

#include "layout/base.h"
#include "layout/spacer.h"

#include "interp/interp.h"
#include "interp/evaluator.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	ErrorOr<EvalResult> vspace(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);
		assert(args[0].isLength());

		auto spacer = std::make_unique<tree::Spacer>(DynLength2d {
			.x = DynLength(),
			.y = args[0].getLength(),
		});

		return EvalResult::ofValue(Value::treeBlockObject(std::move(spacer)));
	}

	ErrorOr<EvalResult> hspace(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);
		assert(args[0].isLength());

		auto spacer = std::make_unique<tree::Spacer>(DynLength2d {
			.x = args[0].getLength(),
			.y = DynLength(),
		});

		return EvalResult::ofValue(Value::treeBlockObject(std::move(spacer)));
	}




	ErrorOr<EvalResult> output_at_absolute_pos_tbo(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		assert(args[1].isTreeBlockObj());

		auto abs_pos = TRY(BS_AbsPosition::unmake(ev, args[0]));

		auto tbo = std::move(args[1]).takeTreeBlockObj();
		TRY(ev->addAbsolutelyPositionedBlockObject(std::move(tbo), abs_pos));

		return EvalResult::ofVoid();
	}

	static ErrorOr<layout::LayoutObject*> get_layout_object_ref(Evaluator* ev, Value& value)
	{
		assert(value.type()->isPointer() && value.type()->pointerElement()->isLayoutObjectRef());

		auto ptr = value.getPointer();
		if(ptr == nullptr)
			return ErrMsg(ev, "unexpected null pointer");

		auto obj = ptr->getLayoutObjectRef();
		if(obj == nullptr)
			return ErrMsg(ev, "unexpected null pointer");

		return Ok(obj);
	}

	static ErrorOr<tree::BlockObject*> get_tbo_ref(Evaluator* ev, Value& value)
	{
		assert(value.type()->isPointer()
			   && (value.type()->pointerElement()->isTreeBlockObj()
				   || value.type()->pointerElement()->isTreeBlockObjRef()));

		auto ptr = value.getPointer();
		if(ptr == nullptr)
			return ErrMsg(ev, "unexpected null pointer");

		if(ptr->type()->isTreeBlockObj())
			return Ok(const_cast<tree::BlockObject*>(&ptr->getTreeBlockObj()));
		else if(ptr->type()->isTreeBlockObjRef())
			return Ok(ptr->getTreeBlockObjectRef());
		else
			return ErrMsg(ev, "unexpected type '{}'", ptr->type());
	}





	ErrorOr<EvalResult> get_layout_object_position(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		if(ev->interpreter()->currentPhase() < ProcessingPhase::PostLayout)
			return ErrMsg(ev, "LayoutObjectRef::position() can only be called during or after `@post`");

		auto layout = ev->pageLayout();
		if(layout == nullptr)
			return ErrMsg(ev, "cannot get object position in this context");

		auto obj = TRY(get_layout_object_ref(ev, args[0]));

		auto pos = obj->resolveAbsPosition(layout);
		return EvalResult::ofValue(BS_AbsPosition::make(ev, pos));
	}

	ErrorOr<EvalResult> offset_object_position(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);

		if(ev->interpreter()->currentPhase() >= ProcessingPhase::Position)
			return ErrMsg(ev, "offset_position() can only be called before `@position`");

		auto offset = TRY(BS_Size2d::unmake(ev, args[1])).resolve(ev->currentStyle());
		if(args[0].isLayoutObjectRef())
		{
			auto obj = TRY(get_layout_object_ref(ev, args[0]));
			obj->addRelativePositionOffset(offset);
		}
		else
		{
			TRY(get_tbo_ref(ev, args[0]))->offsetRelativePosition(offset);
		}

		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> override_object_position(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);

		if(ev->interpreter()->currentPhase() >= ProcessingPhase::Position)
			return ErrMsg(ev, "override_position() can only be called before `@position`");

		auto pos = TRY(BS_AbsPosition::unmake(ev, args[1]));
		if(args[0].isLayoutObjectRef())
		{
			auto obj = TRY(get_layout_object_ref(ev, args[0]));
			obj->overrideAbsolutePosition(pos);
		}
		else
		{
			TRY(get_tbo_ref(ev, args[0]))->overrideAbsolutePosition(pos);
		}

		return EvalResult::ofVoid();
	}
}
