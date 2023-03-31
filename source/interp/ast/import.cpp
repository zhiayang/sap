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
	ErrorOr<TCResult> ImportStmt::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto cs = ts->interpreter();

		auto file = cs->loadFile(file_path);
		auto doc = TRY(frontend::parse(cs->keepStringAlive(this->file_path), file.chars()));

		// add this file to the watchlist
		if(auto e = watch::addFileToWatchList(file_path); e.is_err())
			return ErrMsg(ts, "{}", e.error());

		if(doc.haveDocStart())
			return ErrMsg(ts, "file '{}' contains a '\\start_document', which cannot be imported");

		this->imported_block = std::make_unique<Block>(this->loc());
		this->imported_block->body = std::move(doc).takePreamble();
		this->imported_block->target_scope = QualifiedId { .top_level = true };

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
