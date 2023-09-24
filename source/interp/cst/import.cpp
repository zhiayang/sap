// import.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/cst.h"
#include "interp/interp.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> ImportStmt::evaluate_impl(Evaluator* ev) const
	{
		if(this->imported_block != nullptr)
			TRY(this->imported_block->evaluate(ev));

		return EvalResult::ofVoid();
	}
}
