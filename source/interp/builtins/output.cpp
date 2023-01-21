// output.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/interp.h"
#include "interp/evaluator.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
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

		(*tbo_output_fn)(std::move(args[0]).takeTreeBlockObj());

		return EvalResult::ofVoid();
	}




	ErrorOr<EvalResult> output_at_position_tio(Evaluator* ev, std::vector<Value>& args)
	{
		return ErrMsg(ev, "not implemented");
	}

	ErrorOr<EvalResult> output_at_position_tbo(Evaluator* ev, std::vector<Value>& args)
	{
		return ErrMsg(ev, "not implemented");
	}
}
