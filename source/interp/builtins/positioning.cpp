// positioning.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/base.h"

#include "interp/interp.h"
#include "interp/evaluator.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
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

	ErrorOr<EvalResult> set_layout_object_position(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);

		if(ev->interpreter()->currentPhase() < ProcessingPhase::PostLayout)
			return ErrMsg(ev, "LayoutObjectRef::set_position() can only be called during or after `@post`");

		auto obj = TRY(get_layout_object_ref(ev, args[0]));

		auto pos = TRY(BS_AbsPosition::unmake(ev, args[1]));
		obj->positionAbsolutely(pos);

		return EvalResult::ofVoid();
	}
}