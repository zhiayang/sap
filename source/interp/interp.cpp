// interp.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/interp.h"
#include "interp/state.h"
#include "interp/builtin.h"

namespace sap::interp
{
	Interpreter::Interpreter() : m_top(new DefnTree("")), m_current(m_top.get())
	{
		auto ns_builtin = m_top->lookupOrDeclareNamespace("builtin");

		using BFD = BuiltinFunctionDefn;
		using Param = FunctionDecl::Param;

		auto tio = Type::makeTreeInlineObj();
		auto num = Type::makeNumber();
		auto str = Type::makeString();

		ns_builtin->define(std::make_unique<BFD>("__bold1", Type::makeFunction({ tio }, tio),
			makeParamList(Param { .name = "_", .type = tio }), &builtin::bold1));

		ns_builtin->define(std::make_unique<BFD>("__bold1", Type::makeFunction({ num }, tio),
			makeParamList(Param { .name = "_", .type = num }), &builtin::bold1));

		ns_builtin->define(std::make_unique<BFD>("__bold1", Type::makeFunction({ str }, tio),
			makeParamList(Param { .name = "_", .type = str }), &builtin::bold1));
	}

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

		auto ret = std::unique_ptr<DefnTree>(new DefnTree(std::string(name)));
		return m_children.insert_or_assign(std::string(name), std::move(ret)).first->second.get();
	}

	ErrorOr<std::vector<Declaration*>> DefnTree::lookup(QualifiedId id) const
	{
		auto current = this;
		for(auto& t : id.parents)
		{
			if(auto next = current->lookupNamespace(t); next.ok())
				current = next.unwrap();
			else
				return next.to_err();
		}

		if(auto it = current->m_decls.find(id.name); it != current->m_decls.end())
		{
			std::vector<Declaration*> decls {};
			for(auto& d : it->second)
				decls.push_back(d);

			return Ok(decls);
		}

		return ErrFmt("no declaration named '{}' in '{}'", id.name, current->name());
	}

	ErrorOr<void> DefnTree::declare(Declaration* new_decl)
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
			}
		}

		m_decls[name].push_back(new_decl);
		return Ok();
	}

	ErrorOr<void> DefnTree::define(std::unique_ptr<Definition> defn)
	{
		auto& name = defn->declaration->name;
		if(auto foo = m_decls.find(name); foo != m_decls.end())
		{
			auto& existing_decls = foo->second;
			for(auto& decl : existing_decls)
			{
				// definitions *can* conflict, if they have the same type.
				if(decl->type == defn->declaration->type)
					return ErrFmt("conflicting definition of '{}'", name);
			}
		}

		// steal the ownership of declaration from the definition.
		// the pointer itself is still the same, though; we don't need to re-point it.
		auto decl = defn->declaration.get();

		// somebody must own the definition (not the declaration), so we just own it.
		m_definitions.push_back(std::move(defn));
		m_decls[name].push_back(std::move(decl));
		return Ok();
	}
}
