// typechecker.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "interp/ast.h"
#include "interp/type.h"     // for Type
#include "interp/basedefs.h" // for InlineObject
#include "interp/tc_result.h"

namespace sap::frontend
{
	struct PType;
}

namespace sap::interp
{
	struct DefnTree
	{
		std::string_view name() const { return m_name; }
		DefnTree* parent() const { return m_parent; }

		StrErrorOr<DefnTree*> lookupNamespace(std::string_view name) const;
		DefnTree* lookupOrDeclareNamespace(std::string_view name);

		DefnTree* lookupOrDeclareScope(const std::vector<std::string>& scope, bool is_top_level);

		DefnTree* declareAnonymousNamespace();

		StrErrorOr<std::vector<const Declaration*>> lookup(QualifiedId id) const;

		StrErrorOr<void> declare(const Declaration* decl);

	private:
		explicit DefnTree(std::string name, DefnTree* parent) : m_name(std::move(name)), m_parent(parent) { }

		size_t m_anon_namespace_count = 0;

		std::string m_name;
		util::hashmap<std::string, std::unique_ptr<DefnTree>> m_children;
		util::hashmap<std::string, std::vector<const Declaration*>> m_decls;

		std::vector<Definition*> m_definitions;

		DefnTree* m_parent = nullptr;

		friend struct Typechecker;
	};

	struct Typechecker
	{
		Typechecker();

		DefnTree* top();
		const DefnTree* top() const;

		DefnTree* current();
		const DefnTree* current() const;

		StrErrorOr<const Type*> resolveType(const frontend::PType& ptype);
		StrErrorOr<const Definition*> getDefinitionForType(const Type* type);
		StrErrorOr<void> addTypeDefinition(const Type* type, const Definition* defn);

		[[nodiscard]] util::Defer<> pushTree(DefnTree* tree);
		void popTree();

		StrErrorOr<EvalResult> run(const Stmt* stmt);

		Definition* addBuiltinDefinition(std::unique_ptr<Definition> defn);

		[[nodiscard]] util::Defer<> enterFunctionWithReturnType(const Type* t);
		void leaveFunctionWithReturnType();
		bool isCurrentlyInFunction() const;
		const Type* getCurrentFunctionReturnType() const;

		bool canImplicitlyConvert(const Type* from, const Type* to) const;

	private:
		std::unique_ptr<DefnTree> m_top;

		std::vector<DefnTree*> m_tree_stack;
		std::vector<const Type*> m_expected_return_types;

		std::vector<std::unique_ptr<Definition>> m_builtin_defns;
		std::unordered_map<const Type*, const Definition*> m_type_definitions;
	};
}
