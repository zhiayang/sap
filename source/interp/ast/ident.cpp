// ident.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "location.h" // for error

#include "interp/ast.h"         // for Ident, Declaration
#include "interp/type.h"        // for Type
#include "interp/interp.h"      // for Interpreter, StackFrame, DefnTree
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp
{
	ErrorOr<TCResult> Ident::typecheck_impl(Typechecker* ts, const Type* infer, bool moving) const
	{
		auto tree = ts->current();

		std::vector<const Declaration*> decls {};
		if(auto result = tree->lookup(this->name); result.ok())
			decls = result.take_value();
		else
			return Err(result.take_error());

		assert(decls.size() > 0);
		if(decls.size() == 1)
		{
			auto ret = TRY(decls[0]->typecheck(ts));
			m_resolved_decl = decls[0];

			if(ret.isLValue() && not moving && (ret.type()->isTreeBlockObj() || ret.type()->isTreeInlineObj()))
				return ErrMsg(ts, "{} cannot be copied; use '*' to move or '.clone()' to explicitly clone", ret.type());

			return Ok(ret);
		}
		else
		{
			// TODO: use 'infer' to disambiguate references to functions
			return ErrMsg(ts, "ambiguous '{}'", this->name);
		}
	}

	ErrorOr<EvalResult> Ident::evaluate_impl(Evaluator* ev) const
	{
		// this should have been set by typechecking!
		assert(m_resolved_decl != nullptr);
		assert(m_resolved_decl->definition());

		auto* frame = &ev->frame();
		while(frame != nullptr)
		{
			if(auto value = frame->valueOf(m_resolved_decl->definition()); value != nullptr)
				return EvalResult::ofLValue(*value);

			frame = frame->parent();
		}

		// we didn't find any locals -- check globals.
		if(auto value = ev->getGlobalValue(m_resolved_decl->definition()); value != nullptr)
			return EvalResult::ofLValue(*value);

		return ErrMsg(ev, "use of uninitialised variable '{}'", this->name);
	}
}
