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

		ErrorOr<DefnTree*> lookupNamespace(std::string_view name) const;
		DefnTree* lookupOrDeclareNamespace(std::string_view name);

		DefnTree* lookupOrDeclareScope(const std::vector<std::string>& scope, bool is_top_level);

		DefnTree* declareAnonymousNamespace();

		ErrorOr<std::vector<const Declaration*>> lookup(QualifiedId id) const;

		ErrorOr<void> declare(const Declaration* decl);

	private:
		explicit DefnTree(const Typechecker* ts, std::string name, DefnTree* parent)
		    : m_name(std::move(name))
		    , m_parent(parent)
		    , m_typechecker(ts)
		{
		}

		size_t m_anon_namespace_count = 0;

		std::string m_name;
		util::hashmap<std::string, std::unique_ptr<DefnTree>> m_children;
		util::hashmap<std::string, std::vector<const Declaration*>> m_decls;

		std::vector<Definition*> m_definitions;
		DefnTree* m_parent = nullptr;

		const Typechecker* m_typechecker;
		friend struct Typechecker;
	};

	struct Typechecker
	{
		Typechecker();

		DefnTree* top();
		const DefnTree* top() const;

		DefnTree* current();
		const DefnTree* current() const;

		ErrorOr<const Type*> resolveType(const frontend::PType& ptype);
		ErrorOr<const Definition*> getDefinitionForType(const Type* type);
		ErrorOr<void> addTypeDefinition(const Type* type, const Definition* defn);

		ErrorOr<const DefnTree*> getDefnTreeForType(const Type* type) const;

		[[nodiscard]] Location loc() const;
		[[nodiscard]] util::Defer<> pushLocation(const Location& loc);
		void popLocation();

		[[nodiscard]] util::Defer<> pushTree(DefnTree* tree);
		void popTree();

		ErrorOr<EvalResult> run(const Stmt* stmt);

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

		std::vector<Location> m_location_stack;
	};
}
