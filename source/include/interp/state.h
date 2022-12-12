// state.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>

#include "defs.h"
#include "value.h"
#include "interp.h"

namespace sap::interp
{
	struct DefnTree
	{
		std::string_view name() const { return m_name; }

		ErrorOr<DefnTree*> lookupNamespace(std::string_view name) const;
		DefnTree* lookupOrDeclareNamespace(std::string_view name);

		ErrorOr<std::vector<Declaration*>> lookup(QualifiedId id) const;

		ErrorOr<void> declare(Declaration* decl);
		ErrorOr<void> define(std::unique_ptr<Definition> defn);

	private:
		DefnTree(std::string name) : m_name(std::move(name)) { }

		std::string m_name;
		util::hashmap<std::string, std::unique_ptr<DefnTree>> m_children;
		util::hashmap<std::string, std::vector<Declaration*>> m_decls;

		std::vector<std::unique_ptr<Definition>> m_definitions;

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

	private:
		std::unique_ptr<DefnTree> m_top;
		DefnTree* m_current;
	};
}
