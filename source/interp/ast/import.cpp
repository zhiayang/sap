// import.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/frontend.h"

#include "tree/document.h"
#include "tree/container.h"

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<TCResult> ImportStmt::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		auto cs = ts->interpreter();

		auto file = cs->loadFile(file_path);
		auto doc = frontend::parse(cs->keepStringAlive(this->file_path), file.chars());

		this->imported_block = std::move(doc).takePreamble();

		auto _ = ts->pushTree(ts->top());
		TRY(this->imported_block->typecheck(ts));

		return TCResult::ofVoid();
	}

	ErrorOr<EvalResult> ImportStmt::evaluate_impl(Evaluator* ev) const
	{
		TRY(this->imported_block->evaluate(ev));
		return EvalResult::ofVoid();
	}
}
