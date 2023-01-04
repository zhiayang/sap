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

		DefnTree* top() { return m_top.get(); }
		const DefnTree* top() const { return m_top.get(); }

		DefnTree* current() { return m_tree_stack.back(); }
		const DefnTree* current() const { return m_tree_stack.back(); }

		ErrorOr<const Type*> resolveType(const frontend::PType& ptype);
		ErrorOr<const Definition*> getDefinitionForType(const Type* type);
		ErrorOr<void> addTypeDefinition(const Type* type, const Definition* defn);

		[[nodiscard]] auto pushTree(DefnTree* tree)
		{
			m_tree_stack.push_back(tree);
			return util::Defer([this]() {
				this->popTree();
			});
		}

		void popTree()
		{
			assert(not m_tree_stack.empty());
			m_tree_stack.pop_back();
		}

		ErrorOr<EvalResult> run(const Stmt* stmt);

		Definition* addBuiltinDefinition(std::unique_ptr<Definition> defn);

		[[nodiscard]] auto enterFunctionWithReturnType(const Type* t)
		{
			m_expected_return_types.push_back(t);
			return util::Defer([this]() {
				this->leaveFunctionWithReturnType();
			});
		}

		void leaveFunctionWithReturnType()
		{
			assert(not m_expected_return_types.empty());
			m_expected_return_types.pop_back();
		}

		bool isCurrentlyInFunction() const { return not m_expected_return_types.empty(); }
		const Type* getCurrentFunctionReturnType() const
		{
			assert(this->isCurrentlyInFunction());
			return m_expected_return_types.back();
		}

		bool canImplicitlyConvert(const Type* from, const Type* to) const;

	private:
		std::unique_ptr<DefnTree> m_top;

		std::vector<DefnTree*> m_tree_stack;
		std::vector<const Type*> m_expected_return_types;

		std::vector<std::unique_ptr<Definition>> m_builtin_defns;
		std::unordered_map<const Type*, const Definition*> m_type_definitions;
	};
}
