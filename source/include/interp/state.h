// state.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"   // for hashmap
#include "value.h"  // for Value
#include "interp.h" // for Definition, Declaration, Expr, QualifiedId

#include "interp/basedefs.h" // for InlineObject

namespace sap::interp
{
	struct DefnTree
	{
		std::string_view name() const { return m_name; }
		DefnTree* parent() const { return m_parent; }

		ErrorOr<DefnTree*> lookupNamespace(std::string_view name) const;
		DefnTree* lookupOrDeclareNamespace(std::string_view name);

		ErrorOr<std::vector<const Declaration*>> lookup(QualifiedId id) const;

		ErrorOr<void> declare(const Declaration* decl);
		ErrorOr<void> define(Definition* defn);

	private:
		explicit DefnTree(std::string name, DefnTree* parent) : m_name(std::move(name)), m_parent(parent) { }

		std::string m_name;
		util::hashmap<std::string, std::unique_ptr<DefnTree>> m_children;
		util::hashmap<std::string, std::vector<const Declaration*>> m_decls;

		std::vector<Definition*> m_definitions;

		DefnTree* m_parent = nullptr;

		friend struct Interpreter;
	};


	struct Interpreter
	{
		Interpreter();

		DefnTree* top() { return m_top.get(); }
		const DefnTree* top() const { return m_top.get(); }

		DefnTree* current() { return m_current; }
		const DefnTree* current() const { return m_current; }

		// note: might potentially return null
		std::unique_ptr<tree::InlineObject> run(const Stmt* stmt);
		ErrorOr<Value> evaluate(const Expr* expr);

		Definition* addBuiltinDefinition(std::unique_ptr<Definition> defn);

	private:
		std::unique_ptr<DefnTree> m_top;
		DefnTree* m_current;

		std::vector<std::unique_ptr<Definition>> m_builtin_defns;
	};
}
