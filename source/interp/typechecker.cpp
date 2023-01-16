// typechecker.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/typechecker.h"

namespace sap::interp
{
	Typechecker::Typechecker() : m_top(new DefnTree("__top_level", /* parent: */ nullptr))
	{
		this->pushTree(m_top.get()).cancel();
	}

	DefnTree* Typechecker::top()
	{
		return m_top.get();
	}

	const DefnTree* Typechecker::top() const
	{
		return m_top.get();
	}

	DefnTree* Typechecker::current()
	{
		return m_tree_stack.back();
	}

	const DefnTree* Typechecker::current() const
	{
		return m_tree_stack.back();
	}

	[[nodiscard]] util::Defer<> Typechecker::pushTree(DefnTree* tree)
	{
		m_tree_stack.push_back(tree);
		return util::Defer<>([this]() {
			this->popTree();
		});
	}

	void Typechecker::popTree()
	{
		assert(not m_tree_stack.empty());
		m_tree_stack.pop_back();
	}



	bool Typechecker::canImplicitlyConvert(const Type* from, const Type* to) const
	{
		if(from == to || to->isAny())
			return true;

		else if(from->isNullPtr() && to->isPointer())
			return true;

		else if(from->isPointer() && to->isPointer())
			return from->pointerElement() == to->pointerElement() && from->isMutablePointer();

		else if(from->isInteger() && to->isFloating())
			return true;

		else if(to->isOptional() && to->optionalElement() == from)
			return true;

		else if(to->isOptional() && from->isNullPtr())
			return true;

		else if(to->isOptional() && this->canImplicitlyConvert(from, to->optionalElement()))
			return true;

		else if(from->isEnum() && from->toEnum()->elementType() == to)
			return true;

		return false;
	}

	ErrorOr<const Type*> Typechecker::resolveType(const frontend::PType& ptype)
	{
		using namespace sap::frontend;
		if(ptype.isNamed())
		{
			auto& name = ptype.name();
			if(name.parents.empty() && !name.top_level)
			{
				auto& nn = name.name;
				if(nn == TYPE_INT)
					return Ok(Type::makeInteger());
				else if(nn == TYPE_ANY)
					return Ok(Type::makeAny());
				else if(nn == TYPE_BOOL)
					return Ok(Type::makeBool());
				else if(nn == TYPE_CHAR)
					return Ok(Type::makeChar());
				else if(nn == TYPE_VOID)
					return Ok(Type::makeVoid());
				else if(nn == TYPE_FLOAT)
					return Ok(Type::makeFloating());
				else if(nn == TYPE_STRING)
					return Ok(Type::makeString());
				else if(nn == TYPE_TREE_INLINE)
					return Ok(Type::makeTreeInlineObj());
				else if(nn == TYPE_TREE_BLOCK)
					return Ok(Type::makeTreeBlockObj());
				else if(nn == TYPE_LENGTH)
					return Ok(Type::makeLength());
			}

			auto decl = TRY(this->current()->lookup(name));
			if(decl.size() > 1)
				return ErrFmt("ambiguous type '{}'", name);

			if(auto struct_decl = dynamic_cast<const StructDecl*>(decl[0]); struct_decl)
				return Ok(struct_decl->get_type());

			else if(auto enum_decl = dynamic_cast<const EnumDecl*>(decl[0]); enum_decl)
				return Ok(enum_decl->get_type());
		}
		else if(ptype.isArray())
		{
			return Ok(Type::makeArray(TRY(this->resolveType(ptype.getArrayElement())), ptype.isVariadicArray()));
		}
		else if(ptype.isFunction())
		{
			std::vector<const Type*> params {};
			for(auto& t : ptype.getTypeList())
				params.push_back(TRY(this->resolveType(t)));

			assert(params.size() > 0);
			auto ret = params.back();
			params.pop_back();

			return Ok(Type::makeFunction(std::move(params), ret));
		}
		else if(ptype.isPointer())
		{
			return Ok(Type::makePointer(TRY(this->resolveType(ptype.getPointerElement())), ptype.isMutablePointer()));
		}
		else if(ptype.isOptional())
		{
			return Ok(Type::makeOptional(TRY(this->resolveType(ptype.getOptionalElement()))));
		}


		return ErrFmt("unknown type");
	}


	ErrorOr<const Definition*> Typechecker::getDefinitionForType(const Type* type)
	{
		// first, reduce the type to its base.
		while(true)
		{
			if(type->isPointer())
				type = type->pointerElement();
			else if(type->isOptional())
				type = type->optionalElement();
			else
				break;
		}

		auto it = m_type_definitions.find(type);
		if(it == m_type_definitions.end())
			return ErrFmt("no definition for type '{}'", type);
		else
			return Ok(it->second);
	}

	ErrorOr<void> Typechecker::addTypeDefinition(const Type* type, const Definition* defn)
	{
		if(m_type_definitions.contains(type))
			return ErrFmt("type '{}' was already defined", type);

		m_type_definitions[type] = defn;
		return Ok();
	}

	Definition* Typechecker::addBuiltinDefinition(std::unique_ptr<Definition> defn)
	{
		m_builtin_defns.push_back(std::move(defn));
		return m_builtin_defns.back().get();
	}

	bool Typechecker::isCurrentlyInFunction() const
	{
		return not m_expected_return_types.empty();
	}

	const Type* Typechecker::getCurrentFunctionReturnType() const
	{
		assert(this->isCurrentlyInFunction());
		return m_expected_return_types.back();
	}

	[[nodiscard]] util::Defer<> Typechecker::enterFunctionWithReturnType(const Type* t)
	{
		m_expected_return_types.push_back(t);
		return util::Defer<>([this]() {
			this->leaveFunctionWithReturnType();
		});
	}

	void Typechecker::leaveFunctionWithReturnType()
	{
		assert(not m_expected_return_types.empty());
		m_expected_return_types.pop_back();
	}

}
