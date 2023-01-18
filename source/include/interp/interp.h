// interp.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <deque>

#include "ast.h"   // for Definition, Declaration, QualifiedId
#include "util.h"  // for Defer, hashmap
#include "value.h" // for Value

#include "interp/type.h"        // for Type
#include "interp/basedefs.h"    // for InlineObject
#include "interp/eval_result.h" // for EvalResult

#include "interp/evaluator.h"
#include "interp/typechecker.h"

namespace sap::tree
{
	struct BlockObject;
}

namespace sap::interp
{
	struct Interpreter
	{
		Interpreter();
		ErrorOr<EvalResult> run(const Stmt* stmt);

		Evaluator& evaluator() { return *m_evaluator; }
		Typechecker& typechecker() { return *m_typechecker; }

		tree::BlockObject& leakBlockObject(std::unique_ptr<tree::BlockObject> obj);

		pdf::PdfFont& addLoadedFont(std::unique_ptr<pdf::PdfFont> font);
		ErrorOr<pdf::PdfFont*> getLoadedFontById(int64_t font_id);

	private:
		std::unique_ptr<Typechecker> m_typechecker;
		std::unique_ptr<Evaluator> m_evaluator;

		std::vector<std::unique_ptr<tree::BlockObject>> m_leaked_tbos;

		util::hashmap<int64_t, std::unique_ptr<pdf::PdfFont>> m_loaded_fonts;
	};
}
