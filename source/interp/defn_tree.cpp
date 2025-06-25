// defn_tree.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "util.h"

#include "interp/ast.h"
#include "interp/interp.h"

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

	QualifiedId QualifiedId::named(std::string name)
	{
		return QualifiedId {
			.top_level = false,
			.parents = {},
			.name = std::move(name),
		};
	}

	QualifiedId QualifiedId::withoutLastComponent() const
	{
		if(this->parents.empty())
			sap::internal_error("cannot remove last component without parent scopes");

		auto ret = QualifiedId();
		ret.top_level = this->top_level;
		ret.parents = this->parents;
		ret.name = std::move(this->parents.back());
		ret.parents.pop_back();

		return ret;
	}

	QualifiedId QualifiedId::parentScopeFor(std::string child_name) const
	{
		auto ret = QualifiedId();
		ret.parents = this->parents;
		ret.parents.push_back(this->name);
		ret.top_level = this->top_level;
		ret.name = child_name;

		return ret;
	}

	DefnTree* DefnTree::declareAnonymousNamespace()
	{
		return this->lookupOrDeclareNamespace(zpr::sprint("__$anon{}", m_anon_namespace_count++));
	}

	QualifiedId DefnTree::scopedName(std::string name) const
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
			{
				for(auto& d : decls)
					m_imported_decls[name].push_back(&d);
			}
		}
		else
		{
			if(m_imported_trees.contains(alias))
				return ErrMsg(loc, "using of '{}' conflicts with existing declaration", alias);

			m_imported_trees[alias] = tree;
		}

		return Ok();
	}

	ErrorOr<void> DefnTree::useDeclaration(const Location& loc, const cst::Declaration* decl, const std::string& alias)
	{
		if(alias.empty())
			m_imported_decls[decl->name].push_back(decl);
		else
			m_imported_decls[alias].push_back(decl);

		return Ok();
	}

	cst::Definition* DefnTree::addInstantiatedGeneric(std::unique_ptr<cst::Definition> generic) const
	{
		return m_instantiated_generics.emplace_back(std::move(generic)).get();
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

		const auto& [it, _] = m_children.insert_or_assign(std::string(name), std::move(ret));

		// Iterator contains key/value. Grab the tree value.
		DefnTree* tree = it->second.get();
		return tree;
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

	ErrorOr<std::vector<const cst::Declaration*>> DefnTree::lookup(QualifiedId id) const
	{
		return this->lookup(std::move(id), /* recursive: */ false);
	}

	ErrorOr<std::vector<const cst::Declaration*>> DefnTree::lookupRecursive(QualifiedId id) const
	{
		return this->lookup(std::move(id), /* recursive: */ true);
	}

	ErrorOr<std::vector<const cst::Declaration*>> DefnTree::lookup(QualifiedId id, bool recursive) const
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

		util::hashset<const cst::Declaration*> decls {};

		auto decls_vec = [](util::hashset<const cst::Declaration*>& xs) {
			return std::vector(std::move_iterator(xs.begin()), std::move_iterator(xs.end()));
		};


		while(current != nullptr)
		{
#define CHECK_MAP(map, tok)                                                \
	do                                                                     \
	{                                                                      \
		if(auto it = current->map.find(id.name); it != current->map.end()) \
		{                                                                  \
			for(auto& d : it->second)                                      \
				decls.insert(tok d);                                       \
                                                                           \
			if(not recursive)                                              \
				return Ok(decls_vec(decls));                               \
		}                                                                  \
	} while(false)

			// check non-imported decls first
			CHECK_MAP(m_decls, &);
			CHECK_MAP(m_imported_decls, );
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

	ErrorOr<cst::Declaration*> DefnTree::declare(cst::Declaration new_decl)
	{
		auto check_existing = [this, &new_decl](const auto& existing_decls) -> ErrorOr<void> {
			for(auto& decl_ : existing_decls)
			{
				auto& decl = [&]() -> auto& {
					if constexpr(std::is_pointer_v<std::remove_cvref_t<decltype(decl_)>>)
						return *decl_;
					else
						return decl_;
				}();

				// no error for re-*declaration*, just return ok (and throw away the duplicate decl)
				if(decl == new_decl)
					return Ok();

				if(new_decl.generic_func != nullptr && (decl.generic_func != nullptr || decl.type->isFunction()))
				{
					// ok
				}
				else if(not decl.type->isFunction() || not new_decl.type->isFunction())
				{
					// otherwise, if at least one of them is not a function, they conflict
					return ErrMsg(m_typechecker->loc(), "redeclaration of '{}'", new_decl.name);
				}
				else if((decl.type == new_decl.type) && (decl.name == new_decl.name))
				{
					// otherwise, they are both functions, but we must make sure they are not redefinitions
					// TODO: print the previous one
					return ErrMsg(m_typechecker->loc(), "conflicting declarations of '{}'", new_decl.name);
				}

				// otherwise, we're ok
			}

			return Ok();
		};

		auto& name = new_decl.name;
		if(auto foo = m_decls.find(name); foo != m_decls.end())
			TRY(check_existing(foo->second));

		if(auto foo = m_imported_decls.find(name); foo != m_imported_decls.end())
			TRY(check_existing(foo->second));

		// avoid dangling ref to name after new_decl is moved
		auto& v = m_decls[name];
		auto ret = &v.emplace_back(std::move(new_decl));

		return Ok(ret);
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
