// using.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "sap/frontend.h"

#include "tree/document.h"
#include "tree/container.h"

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp::ast
{
	ErrorOr<TCResult> UsingStmt::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto self = ts->current();
		if(auto maybe_decls = self->lookup(this->module); maybe_decls.ok())
		{
			for(auto& decl : *maybe_decls)
				self->useDeclaration(m_location, decl, this->alias);

			return TCResult::ofVoid<cst::EmptyStmt>(m_location);
		}

		auto tree = self->lookupScope(this->module.parents, this->module.top_level);
		if(tree == nullptr || (tree = tree->lookupNamespace(this->module.name)) == nullptr)
			return ErrMsg(m_location, "no declaration named '{}'", this->module);

		TRY(self->useNamespace(m_location, tree, this->alias));
		return TCResult::ofVoid<cst::EmptyStmt>(m_location);
	}
}
