// start.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/interp.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	ErrorOr<EvalResult> start_document(Evaluator* ev, std::vector<Value>& args)
	{
		return EvalResult::ofVoid();
	}
}
