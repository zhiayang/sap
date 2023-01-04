// expr.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"         // for NumberLit, InlineTreeExpr, StringLit
#include "interp/type.h"        // for Type
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/basedefs.h"    // for InlineObject
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp
{
	ErrorOr<TCResult> InlineTreeExpr::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		return ErrFmt("aoeu");
	}

	ErrorOr<EvalResult> InlineTreeExpr::evaluate(Evaluator* ev) const
	{
		// i guess we have to assert that we cannot evaluate something twice, because
		// otherwise this becomes untenable.
		assert(this->object != nullptr);

		return EvalResult::ofValue(Value::treeInlineObject(std::move(this->object)));
	}
}
