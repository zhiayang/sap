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
	ErrorOr<const Type*> InlineTreeExpr::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return ErrFmt("aoeu");
	}

	ErrorOr<EvalResult> InlineTreeExpr::evaluate(Interpreter* cs) const
	{
		// i guess we have to assert that we cannot evaluate something twice, because
		// otherwise this becomes untenable.
		assert(this->object != nullptr);

		return Ok(EvalResult::of_value(Value::treeInlineObject(std::move(this->object))));
	}

}