// output.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/base.h"

#include "interp/interp.h"
#include "interp/evaluator.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	ErrorOr<EvalResult> set_layout_cursor(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		auto& blk_context = ev->getBlockContext();
		auto fn = blk_context.output_context.set_layout_cursor;
		if(not fn.has_value())
			return ErrMsg(ev, "cannot set layout cursor in this context");

		auto user_pos = TRY(BS_Position::unmake(ev, args[0]));
		auto cursor = blk_context.cursor.moveToPosition(user_pos);
		TRY((*fn)(cursor));

		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> current_layout_position(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 0);
		return EvalResult::ofValue(BS_Position::make(ev, ev->getBlockContext().cursor_ref->position()));
	}



	ErrorOr<EvalResult> output_at_current_tio(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);
		assert(args[0].isTreeInlineObj());

		auto& tio_output_fn = ev->getBlockContext().output_context.add_inline_object;
		if(not tio_output_fn.has_value())
			return ErrMsg(ev, "inline object not allowed in this context");

		auto tios = std::move(args[0]).takeTreeInlineObj();
		for(auto& tio : tios)
			(*tio_output_fn)(std::move(tio));

		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> output_at_current_tbo(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);
		assert(args[0].isTreeBlockObj());

		auto& tbo_output_fn = ev->getBlockContext().output_context.add_block_object;
		if(not tbo_output_fn.has_value())
			return ErrMsg(ev, "block object not allowed in this context");

		auto [_, obj] = TRY((*tbo_output_fn)(std::move(args[0]).takeTreeBlockObj(), std::nullopt));

		auto pos = obj != nullptr //
		             ? obj->layoutPosition()
		             : ev->getBlockContext().cursor_ref->position();


		return EvalResult::ofValue(BS_Position::make(ev, pos));
	}




	ErrorOr<EvalResult> output_at_position_tio(Evaluator* ev, std::vector<Value>& args)
	{
		return ErrMsg(ev, "not implemented");
	}

	ErrorOr<EvalResult> output_at_position_tbo(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		assert(args[1].isTreeBlockObj());

		auto& blk_context = ev->getBlockContext();
		auto& tbo_output_fn = blk_context.output_context.add_block_object;
		if(not tbo_output_fn.has_value())
			return ErrMsg(ev, "block object not allowed in this context");

		// the cursor from userspace is a relative cursor.
		auto user_pos = TRY(BS_Position::unmake(ev, args[0]));
		auto cursor = blk_context.cursor.moveToPosition(user_pos);

		TRY((*tbo_output_fn)(std::move(args[1]).takeTreeBlockObj(), std::move(cursor)));

		return EvalResult::ofVoid();
	}


	ErrorOr<EvalResult> output_at_absolute_pos_tbo(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		assert(args[1].isTreeBlockObj());

		// the cursor from userspace is a relative cursor.
		auto abs_pos = TRY(BS_AbsPosition::unmake(ev, args[0]));
		TRY(ev->addAbsolutelyPositionedBlockObject(std::move(args[1]).takeTreeBlockObj(), abs_pos));

		return EvalResult::ofVoid();
	}
}
