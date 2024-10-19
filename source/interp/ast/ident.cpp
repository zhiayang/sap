// ident.cpp
// Copyright (c) 2022, yuki / zhiayang
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
		auto decls = TRY(tree->lookup(this->name));

		// TODO: use 'infer' to disambiguate references to functions, or
		// have a new kind of TCResult that returns a list of (unresolved) things instead of just one

		assert(decls.size() > 0);
		if(decls.size() == 1)
		{
			auto resolved_decl = decls[0];
			auto dt = resolved_decl->type;

			if(not keep_lvalue && not dt->isCloneable())
				return ErrMsg(ts, "'{}' values cannot be copied; use '*' to move", dt);

			auto ident = std::make_unique<cst::Ident>(m_location, dt, this->name, resolved_decl);
			return TCResult::ofLValue(std::move(ident), resolved_decl->is_mutable);
		}
		else
		{
			return ErrMsg(ts, "ambiguous reference to '{}'", this->name);
		}
	}
}
