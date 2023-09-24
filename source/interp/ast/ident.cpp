// ident.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "location.h" // for error

#include "interp/ast.h"         // for Ident, Declaration
#include "interp/type.h"        // for Type
#include "interp/interp.h"      // for Interpreter, StackFrame, DefnTree
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp::ast
{
	ErrorOr<TCResult> Ident::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto tree = ts->current();

		std::vector<const Declaration*> decls {};
		if(auto result = tree->lookup(this->name); result.ok())
			decls = result.take_value();
		else
			return Err(result.take_error());

		// TODO: use 'infer' to disambiguate references to functions, or
		// have a new kind of TCResult that returns a list of (unresolved) things instead of just one

		assert(decls.size() > 0);
		if(decls.size() == 1)
		{
			auto ret = TRY(decls[0]->typecheck(ts));
			m_resolved_decl = decls[0];

			if(ret.isGeneric())
				return TCResult::ofGeneric(decls[0], /* args: */ {});

			bool cannot_be_copied =
			    (ret.type()->isTreeBlockObj() || ret.type()->isTreeInlineObj() || ret.type()->isLayoutObject());

			if(ret.isLValue() && not keep_lvalue && cannot_be_copied)
				return ErrMsg(ts, "'{}' values cannot be copied; use '*' to move", ret.type());

			return Ok(ret);
		}
		else
		{
			return ErrMsg(ts, "ambiguous reference to '{}'", this->name);
		}
	}
}
