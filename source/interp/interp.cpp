// interp.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "location.h" // for error

#include "tree/base.h"

#include "interp/ast.h"         // for Definition, makeParamList, Expr, Stmt
#include "interp/type.h"        // for Type
#include "interp/interp.h"      // for StackFrame, Interpreter, DefnTree
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp
{
	extern void defineBuiltins(Interpreter* interp, DefnTree* builtin_ns);

	Interpreter::Interpreter() : m_typechecker(new Typechecker()), m_evaluator(new Evaluator())
	{
		defineBuiltins(this, m_typechecker->top()->lookupOrDeclareNamespace("builtin"));
	}

	StrErrorOr<EvalResult> Interpreter::run(const Stmt* stmt)
	{
		if(auto res = stmt->typecheck(m_typechecker.get()); res.is_err())
			error(stmt->loc(), "{}", res.take_error());

		return stmt->evaluate(m_evaluator.get());
	}
}
