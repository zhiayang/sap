// defn_tree.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for hashmap

#include "interp/ast.h"    // for FunctionDecl, Declaration, QualifiedId
#include "interp/interp.h" // for DefnTree

namespace sap::interp
{
	ErrorOr<DefnTree*> DefnTree::lookupNamespace(std::string_view name) const
	{
		if(auto it = m_children.find(name); it != m_children.end())
			return Ok(it->second.get());

		return ErrMsg(m_typechecker->loc(), "namespace '{}' was not found in {}", name, m_name);
	}

	DefnTree* DefnTree::lookupOrDeclareNamespace(std::string_view name)
	{
		if(auto it = m_children.find(name); it != m_children.end())
			return it->second.get();

		auto ret = std::unique_ptr<DefnTree>(new DefnTree(m_typechecker, std::string(name), /* parent: */ this));
		return m_children.insert_or_assign(std::string(name), std::move(ret)).first->second.get();
	}

	DefnTree* DefnTree::declareAnonymousNamespace()
	{
		return this->lookupOrDeclareNamespace(zpr::sprint("__$anon{}", m_anon_namespace_count++));
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

	ErrorOr<std::vector<const Declaration*>> DefnTree::lookup(QualifiedId id) const
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
				if(current->lookupNamespace(id.parents[0]).ok())
					break;

				current = current->parent();
				if(current == nullptr)
					return ErrMsg(m_typechecker->loc(), "no such namespace '{}'", id.parents[0]);
			}
		}

		for(auto& t : id.parents)
		{
			if(auto next = current->lookupNamespace(t); next.ok())
				current = next.unwrap();
			else
				return next.to_err();
		}

		while(current != nullptr)
		{
			if(auto it = current->m_decls.find(id.name); it != current->m_decls.end())
			{
				std::vector<const Declaration*> decls {};
				for(auto& d : it->second)
					decls.push_back(d);

				return Ok(decls);
			}

			current = current->parent();
		}

		return ErrMsg(m_typechecker->loc(), "no declaration named '{}' (search started at '{}')", id.name, m_name);
	}

	static bool function_decls_conflict(const FunctionDecl* a, const FunctionDecl* b)
	{
		if(a->name != b->name || a->params().size() != b->params().size())
			return false;

		return a->get_type() == b->get_type();
	}


	ErrorOr<void> DefnTree::declare(const Declaration* new_decl)
	{
		auto& name = new_decl->name;

		if(auto foo = m_decls.find(name); foo != m_decls.end())
		{
			auto& existing_decls = foo->second;
			for(auto& decl : existing_decls)
			{
				// no error for re-*declaration*, just return ok (and throw away the duplicate decl)
				if(decl == new_decl)
					return Ok();

				auto af = dynamic_cast<const FunctionDecl*>(decl);
				auto bf = dynamic_cast<const FunctionDecl*>(new_decl);

				if(af == nullptr || bf == nullptr)
				{
					// otherwise, if at least one of them is not a function, they conflict
					return ErrMsg(m_typechecker->loc(), "redeclaration of '{}'", name);
				}
				else if(function_decls_conflict(af, bf))
				{
					// otherwise, they are both functions, but we must make sure they are not redefinitions
					// TODO: print the previous one
					return ErrMsg(m_typechecker->loc(), "conflicting declarations of '{}'", name);
				}

				// otherwise, we're ok
			}
		}

		m_decls[name].push_back(new_decl);
		const_cast<Declaration*>(new_decl)->declareAt(this);

		return Ok();
	}
}
