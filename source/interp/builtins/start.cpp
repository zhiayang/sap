// start.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "interp/interp.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	ErrorOr<EvalResult> start_document(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		DocumentSettings settings {};
		if(args[0].haveOptionalValue())
			settings = TRY(BS_DocumentSettings::unmake(ev, **args[0].getOptional()));

		return EvalResult::ofValue(BS_DocumentSettings::make(ev, std::move(settings)));
	}

	ErrorOr<EvalResult> request_layout(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 0);

		ev->requestLayout();
		return EvalResult::ofVoid();
	}
}
