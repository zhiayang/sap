// import.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/config.h"
#include "sap/frontend.h"

#include "tree/document.h"
#include "tree/container.h"

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> ImportStmt::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto cs = ts->interpreter();

		auto resolved = TRY(sap::paths::resolveLibrary(m_location, this->file_path));
		if(auto dir = stdfs::path(resolved); stdfs::is_directory(dir))
		{
			auto name = dir.stem();
			name.replace_extension(".sap");

			auto lib_file = dir / name;
			if(not stdfs::exists(lib_file))
				return ErrMsg(ts, "imported folder '{}' does not contain a '{}' file", resolved, name.string());

			resolved = lib_file.string();
		}

		if(cs->wasFileImported(resolved))
			return TCResult::ofVoid<cst::EmptyStmt>(m_location);

		cs->addImportedFile(resolved);

		if(auto e = watch::addFileToWatchList(resolved); e.is_err())
			return ErrMsg(ts, "{}", e.error());

		auto file = cs->loadFile(resolved);
		auto doc = TRY(frontend::parse(cs->keepStringAlive(resolved), file.chars()));

		if(doc.haveDocStart())
			return ErrMsg(ts, "file '{}' contains a '\\start_document', which cannot be imported");

		auto imported_block = std::make_unique<Block>(this->loc());
		imported_block->body = std::move(doc).takePreamble();
		imported_block->target_scope = QualifiedId { .top_level = true };

		auto _ = ts->pushTree(ts->top());
		return TCResult::ofVoid<cst::ImportStmt>(m_location, TRY(imported_block->typecheck(ts)).take<cst::Block>());
	}
}
