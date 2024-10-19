// fstring.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> FStringExpr::evaluate_impl(Evaluator* ev) const
	{
		std::u32string str {};

		for(auto& part : this->parts)
		{
			if(auto expr = part.maybe_right(); expr != nullptr)
				str += TRY_VALUE((*expr)->evaluate(ev)).getUtf32String();
			else
				str += part.left();
		}

		return EvalResult::ofValue(Value::string(std::move(str)));
	}
}
