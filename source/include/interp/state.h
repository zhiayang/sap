// state.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"   // for hashmap
#include "value.h"  // for Value
#include "interp.h" // for Definition, Declaration, Expr, QualifiedId

#include "interp/type.h"     // for Type
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

	struct StackFrame
	{
		StackFrame* parent() const { return m_parent; }

		Value* valueOf(const Definition* defn)
		{
			if(auto it = m_values.find(defn); it == m_values.end())
				return nullptr;
			else
				return &it->second;
		}

		void setValue(const Definition* defn, Value value) { m_values[defn] = std::move(value); }

	private:
		explicit StackFrame(StackFrame* parent) : m_parent(parent) { }

		friend struct Interpreter;

		StackFrame* m_parent = nullptr;
		std::unordered_map<const Definition*, Value> m_values;
	};

	struct Interpreter
	{
		Interpreter();

		DefnTree* top() { return m_top.get(); }
		const DefnTree* top() const { return m_top.get(); }

		DefnTree* current() { return m_current; }
		const DefnTree* current() const { return m_current; }

		ErrorOr<void> run(const Stmt* stmt);
		ErrorOr<std::optional<Value>> evaluate(const Expr* expr);

		Definition* addBuiltinDefinition(std::unique_ptr<Definition> defn);

		StackFrame& frame();
		StackFrame& pushFrame();
		void popFrame();

		bool canImplicitlyConvert(const Type* from, const Type* to) const;

	private:
		std::unique_ptr<DefnTree> m_top;
		DefnTree* m_current;

		std::vector<std::unique_ptr<Definition>> m_builtin_defns;
		std::vector<std::unique_ptr<StackFrame>> m_stack_frames;
	};
}
