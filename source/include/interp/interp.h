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

namespace sap::interp
{
	struct Interpreter
	{
		Interpreter();
		ErrorOr<EvalResult> run(const Stmt* stmt);

		Evaluator& evaluator() { return *m_evaluator; }
		Typechecker& typechecker() { return *m_typechecker; }

	private:
		std::unique_ptr<Typechecker> m_typechecker;
		std::unique_ptr<Evaluator> m_evaluator;
	};
}
