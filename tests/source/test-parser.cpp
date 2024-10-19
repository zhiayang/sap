// test-parser.cpp
// Copyright (c) 2024, yuki
// SPDX-License-Identifier: Apache-2.0

#include "tester.h"

#include "sap/frontend.h"

#include "interp/ast.h"
#include "tree/document.h"
#include "tree/container.h"

namespace test
{
	static void run_test(Context& ctx, const stdfs::path& test)
	{
		auto contents = util::readEntireFile(test);
		auto doc = sap::frontend::parse(test.string(), contents.span().chars());
		if(doc.is_err())
			return ctx.failed++, void();

		auto& preamble = doc->preamble();
		for(auto& p : preamble)
			zpr::println("{}", dumpStmt(p.get()).serialise(true));
	}

	void test_parser(Context& ctx, const stdfs::path& test_dir)
	{
		auto test_cases = find_files_ext(test_dir / "lang" / "parser", ".sap");
		for(auto& tc : test_cases)
			run_test(ctx, tc);
	}
}
