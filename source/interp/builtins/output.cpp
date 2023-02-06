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
	ErrorOr<EvalResult> output_at_absolute_pos_tbo(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 2);
		assert(args[1].isTreeBlockObj());

		auto abs_pos = TRY(BS_AbsPosition::unmake(ev, args[0]));
		TRY(ev->addAbsolutelyPositionedBlockObject(std::move(args[1]).takeTreeBlockObj(), abs_pos));

		return EvalResult::ofVoid();
	}
}
