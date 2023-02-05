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
		return ErrMsg(ev, "cannot set layout cursor in this context");
	}

	ErrorOr<EvalResult> current_layout_position(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 0);
		return EvalResult::ofValue(BS_Position::make(ev, {}));
	}



	ErrorOr<EvalResult> output_at_current_tio(Evaluator* ev, std::vector<Value>& args)
	{
		return ErrMsg(ev, "not implemented");
	}

	ErrorOr<EvalResult> output_at_current_tbo(Evaluator* ev, std::vector<Value>& args)
	{
		return ErrMsg(ev, "not implemented");
	}




	ErrorOr<EvalResult> output_at_position_tio(Evaluator* ev, std::vector<Value>& args)
	{
		return ErrMsg(ev, "not implemented");
	}

	ErrorOr<EvalResult> output_at_position_tbo(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		assert(args[1].isTreeBlockObj());

		auto abs_pos = TRY(BS_Position::unmake(ev, args[0]));
		TRY(ev->addPositionedBlockObject(std::move(args[1]).takeTreeBlockObj(), abs_pos));

		return EvalResult::ofVoid();
	}


	ErrorOr<EvalResult> output_at_absolute_pos_tbo(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		assert(args[1].isTreeBlockObj());

		auto abs_pos = TRY(BS_AbsPosition::unmake(ev, args[0]));
		TRY(ev->addPositionedBlockObject(std::move(args[1]).takeTreeBlockObj(), abs_pos));

		return EvalResult::ofVoid();
	}
}
