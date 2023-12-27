// typechecker.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <list>

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

		DefnTree* lookupNamespace(std::string_view name) const;
		DefnTree* lookupOrDeclareNamespace(std::string_view name);

		DefnTree* lookupScope(const std::vector<std::string>& scope, bool is_top_level);
		DefnTree* lookupOrDeclareScope(const std::vector<std::string>& scope, bool is_top_level);

		DefnTree* declareAnonymousNamespace();

		ErrorOr<std::vector<const cst::Declaration*>> lookup(QualifiedId id) const;
		ErrorOr<std::vector<const cst::Declaration*>> lookupRecursive(QualifiedId id) const;

		ErrorOr<cst::Declaration*> declare(cst::Declaration decl);
		QualifiedId scopedName(std::string name) const;

		void dump(int indent = 0) const;

		ErrorOr<void> useNamespace(const Location& loc, DefnTree* other, const std::string& alias);
		ErrorOr<void> useDeclaration(const Location& loc, const cst::Declaration* other, const std::string& alias);

	private:
		explicit DefnTree(const Typechecker* ts, std::string name, DefnTree* parent)
		    : m_name(std::move(name)), m_parent(parent), m_typechecker(ts)
		{
		}

		ErrorOr<std::vector<const cst::Declaration*>> lookup(QualifiedId id, bool recursive) const;

	private:
		size_t m_anon_namespace_count = 0;

		std::string m_name;
		util::hashmap<std::string, std::unique_ptr<DefnTree>> m_children;
		util::hashmap<std::string, std::list<cst::Declaration>> m_decls;

		util::hashmap<std::string, DefnTree*> m_imported_trees;
		util::hashmap<std::string, std::vector<const cst::Declaration*>> m_imported_decls;

		DefnTree* m_parent = nullptr;

		const Typechecker* m_typechecker;
		friend struct Typechecker;
	};

	struct Typechecker
	{
		explicit Typechecker(Interpreter* interp);
		Interpreter* interpreter() { return m_interp; }

		DefnTree* top();
		const DefnTree* top() const;

		DefnTree* current();
		const DefnTree* current() const;

		[[nodiscard]] ErrorOr<const Type*> resolveType(const frontend::PType& ptype);
		[[nodiscard]] ErrorOr<const cst::Definition*> getDefinitionForType(const Type* type);
		[[nodiscard]] ErrorOr<void> addTypeDefinition(const Type* type, const cst::Definition* defn);

		ErrorOr<const DefnTree*> getDefnTreeForType(const Type* type) const;

		[[nodiscard]] Location loc() const;
		[[nodiscard]] util::Defer<> pushLocation(const Location& loc);
		void popLocation();

		[[nodiscard]] util::Defer<> pushTree(DefnTree* tree);
		void popTree();

		ErrorOr<EvalResult> run(const ast::Stmt* stmt);

		cst::Definition* addBuiltinDefinition(std::unique_ptr<cst::Definition> defn);

		[[nodiscard]] util::Defer<> enterFunctionWithReturnType(const Type* t);
		void leaveFunctionWithReturnType();
		bool isCurrentlyInFunction() const;
		const Type* getCurrentFunctionReturnType() const;

		[[nodiscard]] util::Defer<> enterLoopBody();
		bool isCurrentlyInLoopBody() const;

		[[nodiscard]] util::Defer<> pushStructFieldContext(const StructType* str);
		const StructType* getStructFieldContext() const;
		bool haveStructFieldContext() const;

		bool canImplicitlyConvert(const Type* from, const Type* to) const;

		std::string getTemporaryName();

	private:
		Interpreter* m_interp;
		std::unique_ptr<DefnTree> m_top;

		std::vector<DefnTree*> m_tree_stack;
		std::vector<const Type*> m_expected_return_types;
		std::vector<const StructType*> m_struct_field_context_stack;

		std::vector<std::unique_ptr<cst::Definition>> m_builtin_defns;
		util::hashmap<const Type*, const cst::Definition*> m_type_definitions;

		std::vector<Location> m_location_stack;
		size_t m_loop_body_nesting;
		size_t m_tmp_name_counter;
	};
}
