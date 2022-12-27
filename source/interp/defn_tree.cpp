// defn_tree.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<DefnTree*> DefnTree::lookupNamespace(std::string_view name) const
	{
		if(auto it = m_children.find(name); it != m_children.end())
			return Ok(it->second.get());

		return ErrFmt("namespace '{}' was not found in {}", name, m_name);
	}

	DefnTree* DefnTree::lookupOrDeclareNamespace(std::string_view name)
	{
		if(auto it = m_children.find(name); it != m_children.end())
			return it->second.get();

		auto ret = std::unique_ptr<DefnTree>(new DefnTree(std::string(name), /* parent: */ this));
		return m_children.insert_or_assign(std::string(name), std::move(ret)).first->second.get();
	}

	DefnTree* DefnTree::declareAnonymousNamespace()
	{
		return this->lookupOrDeclareNamespace(zpr::sprint("__$anon{}", m_anon_namespace_count++));
	}

	ErrorOr<std::vector<const Declaration*>> DefnTree::lookup(QualifiedId id) const
	{
		auto current = this;

		// look upwards at our parents to find something that matches the first thing
		if(not id.parents.empty())
		{
			while(true)
			{
				if(current->lookupNamespace(id.parents[0]).ok())
					break;

				current = current->parent();
				if(current == nullptr)
					return ErrFmt("no such namespace '{}'", id.parents[0]);
			}
		}

		for(auto& t : id.parents)
		{
			if(auto next = current->lookupNamespace(t); next.ok())
				current = next.unwrap();
			else
				return next.to_err();
		}

		if(auto it = current->m_decls.find(id.name); it != current->m_decls.end())
		{
			std::vector<const Declaration*> decls {};
			for(auto& d : it->second)
				decls.push_back(d);

			return Ok(decls);
		}

		return ErrFmt("no declaration named '{}' in '{}'", id.name, current->name());
	}

	static bool function_decls_conflict(const FunctionDecl* a, const FunctionDecl* b)
	{
		if(a->name != b->name || a->params().size() != b->params().size())
			return false;

		for(size_t i = 0; i < a->params().size(); i++)
		{
			auto& ap = a->params()[i];
			auto& bp = b->params()[i];

			if(ap.type != bp.type)
				return false;
		}

		return true;
	}


	ErrorOr<void> DefnTree::declare(const Declaration* new_decl)
	{
		auto& name = new_decl->name;
		zpr::println("declaring {}: {}", name, (void*) new_decl);

		if(auto foo = m_decls.find(name); foo != m_decls.end())
		{
			zpr::println("finding '{}' {} dupes", name, foo->second.size());

			auto& existing_decls = foo->second;
			for(auto& decl : existing_decls)
			{
				zpr::println(">> {}", (void*) decl);

				if(decl == new_decl)
				{
					// no error for re-*declaration*, just return ok (and throw away the duplicate decl)
					return Ok();
				}

				auto af = dynamic_cast<const FunctionDecl*>(decl);
				auto bf = dynamic_cast<const FunctionDecl*>(new_decl);

				if(af == nullptr || bf == nullptr)
				{
					// otherwise, if at least one of them is not a function, they conflict
					return ErrFmt("redeclaration of '{}' as a different symbol", name);
				}
				else if(function_decls_conflict(af, bf))
				{
					// otherwise, they are both functions, but we must make sure they are not redefinitions
					// TODO: print the previous one
					return ErrFmt("conflicting declarations of '{}'", name);
				}

				// otherwise, we're ok
			}
		}

		m_decls[name].push_back(new_decl);
		return Ok();
	}

	ErrorOr<void> DefnTree::define(Definition* defn)
	{
		TRY(this->declare(defn->declaration.get()));
		m_definitions.push_back(defn);
		return Ok();
	}
}
