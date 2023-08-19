// defn_tree.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for hashmap

#include "interp/ast.h"    // for FunctionDecl, Declaration, QualifiedId
#include "interp/interp.h" // for DefnTree

namespace sap::interp
{
	std::string QualifiedId::str() const
	{
		std::string ret {};
		for(const auto& p : this->parents)
			ret += (p + "::");

		ret += this->name;
		return ret;
	}

	DefnTree* DefnTree::declareAnonymousNamespace()
	{
		return this->lookupOrDeclareNamespace(zpr::sprint("__$anon{}", m_anon_namespace_count++));
	}

	QualifiedId DefnTree::scopeName(std::string name) const
	{
		QualifiedId ret {};
		ret.top_level = true;
		ret.name = std::move(name);

		for(auto tree = this; tree != nullptr; tree = tree->parent())
			ret.parents.push_back(tree->m_name);

		std::reverse(ret.parents.begin(), ret.parents.end());
		return ret;
	}

	ErrorOr<void> DefnTree::useNamespace(const Location& loc, DefnTree* tree, const std::string& alias)
	{
		if(alias.empty())
		{
			// import all the decls in the guy into ourselves.
			for(auto& [name, decls] : tree->m_decls)
				m_imported_decls[name] = decls;
		}
		else
		{
			if(m_imported_trees.contains(alias))
				return ErrMsg(loc, "using of '{}' conflicts with existing declaration", alias);

			m_imported_trees[alias] = tree;
		}

		return Ok();
	}

	ErrorOr<void> DefnTree::useDeclaration(const Location& loc, const ast::Declaration* decl, const std::string& alias)
	{
		if(alias.empty())
			m_imported_decls[decl->name].push_back(decl);
		else
			m_imported_decls[alias].push_back(decl);

		return Ok();
	}




	DefnTree* DefnTree::lookupNamespace(std::string_view name) const
	{
		if(auto it = m_imported_trees.find(name); it != m_imported_trees.end())
			return it->second;

		if(auto it = m_children.find(name); it != m_children.end())
			return it->second.get();

		return nullptr;
	}

	DefnTree* DefnTree::lookupOrDeclareNamespace(std::string_view name)
	{
		// FIXME: not sure if we should be checking the imported trees here...
		if(auto it = m_imported_trees.find(name); it != m_imported_trees.end())
			return it->second;

		if(auto it = m_children.find(name); it != m_children.end())
			return it->second.get();

		auto ret = std::unique_ptr<DefnTree>(new DefnTree(m_typechecker, std::string(name), /* parent: */ this));
		return m_children.insert_or_assign(std::string(name), std::move(ret)).first->second.get();
	}



	DefnTree* DefnTree::lookupScope(const std::vector<std::string>& scope, bool is_top_level)
	{
		auto current = this;

		if(is_top_level)
		{
			while(current->parent() != nullptr)
				current = current->parent();
		}

		// this is always additive, so don't do any looking upwards
		for(auto& ns : scope)
		{
			if(auto tmp = current->lookupNamespace(ns))
				current = tmp;
			else
				return nullptr;
		}

		return current;
	}

	DefnTree* DefnTree::lookupOrDeclareScope(const std::vector<std::string>& scope, bool is_top_level)
	{
		auto current = this;

		if(is_top_level)
		{
			while(current->parent() != nullptr)
				current = current->parent();
		}

		// this is always additive, so don't do any looking upwards
		for(auto& ns : scope)
			current = current->lookupOrDeclareNamespace(ns);

		return current;
	}

	ErrorOr<std::vector<const ast::Declaration*>> DefnTree::lookup(QualifiedId id) const
	{
		return this->lookup(std::move(id), /* recursive: */ false);
	}

	ErrorOr<std::vector<const ast::Declaration*>> DefnTree::lookupRecursive(QualifiedId id) const
	{
		return this->lookup(std::move(id), /* recursive: */ true);
	}

	ErrorOr<std::vector<const ast::Declaration*>> DefnTree::lookup(QualifiedId id, bool recursive) const
	{
		auto current = this;

		if(id.top_level)
		{
			while(current->parent() != nullptr)
				current = current->parent();
		}

		// look upwards at our parents to find something that matches the first thing
		if(not id.parents.empty())
		{
			while(true)
			{
				if(current->lookupNamespace(id.parents[0]))
					break;

				current = current->parent();
				if(current == nullptr)
					return ErrMsg(m_typechecker->loc(), "no such namespace '{}'", id.parents[0]);
			}
		}

		for(auto& t : id.parents)
		{
			if(auto next = current->lookupNamespace(t))
				current = next;
			else
				return ErrMsg(m_typechecker->loc(), "undeclared namespace '{}'", t);
		}

		util::hashset<const ast::Declaration*> decls {};

		auto decls_vec = [](util::hashset<const ast::Declaration*>& xs) {
			return std::vector(std::move_iterator(xs.begin()), std::move_iterator(xs.end()));
		};


		while(current != nullptr)
		{
#define CHECK_MAP(map)                                                     \
	do                                                                     \
	{                                                                      \
		if(auto it = current->map.find(id.name); it != current->map.end()) \
		{                                                                  \
			for(auto& d : it->second)                                      \
				decls.insert(d);                                           \
			if(not recursive)                                              \
				return Ok(decls_vec(decls));                               \
		}                                                                  \
	} while(false)

			// check imported decls first
			CHECK_MAP(m_imported_decls);
			CHECK_MAP(m_decls);
			CHECK_MAP(m_generic_decls);

#undef CHECK_MAP

			// if the id was qualified, then we shouldn't search in parents.
			if(not id.parents.empty() || current->parent() == nullptr)
				break;

			current = current->parent();
		}

		if(not decls.empty())
			return Ok(decls_vec(decls));

		return ErrMsg(m_typechecker->loc(), "no declaration named '{}' (search started at '{}')", id.name, m_name);
	}

	static bool function_decls_conflict(const ast::FunctionDecl* a, const ast::FunctionDecl* b)
	{
		if(a->name != b->name || a->params().size() != b->params().size())
			return false;

		return a->get_type() == b->get_type();
	}

	ErrorOr<void> DefnTree::declareGeneric(const ast::Declaration* decl)
	{
		// TODO: check for conflicts
		m_generic_decls[decl->name].push_back(decl);
		return Ok();
	}

	ErrorOr<void> DefnTree::declare(const ast::Declaration* new_decl)
	{
		auto check_existing =
		    [this, new_decl](const std::vector<const ast::Declaration*>& existing_decls) -> ErrorOr<void> {
			for(auto& decl : existing_decls)
			{
				// no error for re-*declaration*, just return ok (and throw away the duplicate decl)
				if(decl == new_decl)
					return Ok();

				auto af = dynamic_cast<const ast::FunctionDecl*>(decl);
				auto bf = dynamic_cast<const ast::FunctionDecl*>(new_decl);

				if(af == nullptr || bf == nullptr)
				{
					// otherwise, if at least one of them is not a function, they conflict
					return ErrMsg(m_typechecker->loc(), "redeclaration of '{}'", new_decl->name);
				}
				else if(function_decls_conflict(af, bf))
				{
					// otherwise, they are both functions, but we must make sure they are not redefinitions
					// TODO: print the previous one
					return ErrMsg(m_typechecker->loc(), "conflicting declarations of '{}'", new_decl->name);
				}

				// otherwise, we're ok
			}

			return Ok();
		};

		auto& name = new_decl->name;
		if(auto foo = m_decls.find(name); foo != m_decls.end())
			TRY(check_existing(foo->second));

		if(auto foo = m_imported_decls.find(name); foo != m_imported_decls.end())
			TRY(check_existing(foo->second));

		m_decls[name].push_back(new_decl);
		const_cast<ast::Declaration*>(new_decl)->declareAt(this);

		return Ok();
	}

	void DefnTree::dump(int indent) const
	{
		auto x = std::string(static_cast<size_t>(indent * 2), ' ');

		zpr::println("{}name = '{}'", x, m_name);
		zpr::println("{}defns ({}):", x, m_decls.size());
		for(auto& [n, d] : m_decls)
			zpr::println("  {}'{}'{}", x, n, d.size() == 1 ? "" : zpr::sprint(" ({} overloads)", d.size()));

		zpr::println("{}subs ({}):", x, m_children.size());
		for(auto& child : m_children)
			child.second->dump(indent + 1);
	}
}
