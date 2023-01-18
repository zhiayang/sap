// font_finder.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/interp.h"
#include "interp/builtin_fns.h"

namespace sap::interp::builtin
{
	ErrorOr<EvalResult> find_font(Evaluator* ev, std::vector<Value>& args)
	{
		return ErrMsg(ev, "help");
	}
}
