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
	ErrorOr<const Type*> Ident::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		auto tree = cs->current();

		std::vector<const Declaration*> decls {};
		if(auto result = tree->lookup(this->name); result.ok())
			decls = result.take_value();
		else
			error(this->location, "{}", result.take_error());

		assert(decls.size() > 0);
		if(decls.size() == 1)
		{
			m_type = TRY(decls[0]->typecheck(cs));
			m_resolved_decl = decls[0];

			return Ok(m_type);
		}
		else
		{
			// TODO: use 'infer' to disambiguate references to functions
			return ErrFmt("ambiguous '{}'", this->name);
		}
	}

	ErrorOr<EvalResult> Ident::evaluate(Interpreter* cs) const
	{
		// this should have been set by typechecking!
		assert(m_resolved_decl != nullptr);
		assert(m_resolved_decl->resolved_defn);

		auto* frame = &cs->frame();
		while(true)
		{
			if(auto value = frame->valueOf(m_resolved_decl->resolved_defn); value != nullptr)
				return Ok(EvalResult::of_value(std::move(*value)));

			if(frame->parent())
				frame = frame->parent();
			else
				return ErrFmt("use of uninitialised variable '{}'", this->name);
		}
	}
}
