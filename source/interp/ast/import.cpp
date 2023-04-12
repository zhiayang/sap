// import.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/paths.h"
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

		auto resolved = TRY(sap::paths::resolveLibrary(m_location, this->file_path));
		if(auto dir = stdfs::path(resolved); stdfs::is_directory(dir))
		{
			auto lib_file = dir / "module.sap";
			if(not stdfs::exists(lib_file))
				return ErrMsg(ts, "imported folder '{}' does not contain a 'module.sap' file", resolved);

			resolved = lib_file.string();
		}

		if(cs->wasFileImported(resolved))
			return TCResult::ofVoid();

		cs->addImportedFile(resolved);

		if(auto e = watch::addFileToWatchList(resolved); e.is_err())
			return ErrMsg(ts, "{}", e.error());

		auto file = cs->loadFile(resolved);
		auto doc = TRY(frontend::parse(cs->keepStringAlive(resolved), file.chars()));

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
		if(this->imported_block != nullptr)
			TRY(this->imported_block->evaluate(ev));

		return EvalResult::ofVoid();
	}
}
