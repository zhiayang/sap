// typechecker.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"

#include "interp/ast.h"
#include "interp/typechecker.h"

namespace sap::interp
{
	Typechecker::Typechecker(Interpreter* interp)
	    : m_interp(interp)
	    , m_top(new DefnTree(this, "__top_level", /* parent: */ nullptr))
	    , m_loop_body_nesting(0)
	    , m_tmp_name_counter(0)
	{
		this->pushTree(m_top.get()).cancel();
	}

	std::string Typechecker::getTemporaryName()
	{
		return zpr::sprint("__tmp_{}", ++m_tmp_name_counter);
	}

	[[nodiscard]] Location Typechecker::loc() const
	{
		return m_location_stack.back();
	}

	[[nodiscard]] util::Defer<> Typechecker::pushLocation(const Location& loc)
	{
		m_location_stack.push_back(loc);
		return util::Defer([this]() { this->popLocation(); });
	}

	void Typechecker::popLocation()
	{
		assert(m_location_stack.size() > 0);
		m_location_stack.pop_back();
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
		return util::Defer<>([this]() { this->popTree(); });
	}

	void Typechecker::popTree()
	{
		assert(not m_tree_stack.empty());
		m_tree_stack.pop_back();
	}

	[[nodiscard]] util::Defer<> Typechecker::pushStructFieldContext(const StructType* str)
	{
		m_struct_field_context_stack.push_back(str);
		return util::Defer<>([this]() { m_struct_field_context_stack.pop_back(); });
	}

	const StructType* Typechecker::getStructFieldContext() const
	{
		assert(not m_struct_field_context_stack.empty());
		return m_struct_field_context_stack.back();
	}

	bool Typechecker::haveStructFieldContext() const
	{
		return not m_struct_field_context_stack.empty();
	}




	bool Typechecker::canImplicitlyConvert(const Type* from, const Type* to) const
	{
		if(from == to || to->isAny())
			return true;

		else if(from->isNullPtr() && to->isPointer())
			return true;

		else if(from->isPointer() && to->isPointer() && from->pointerElement() == to->pointerElement()
		        && from->isMutablePointer())
			return true;

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

		// TODO: these are all hacky! we don't have generics yet.
		else if(from->isArray() && to->isArray() && (from->arrayElement()->isVoid() || to->arrayElement()->isVoid()))
			return true;

		else if(
		    from->isPointer() && to->isPointer() && from->pointerElement()->isArray() && to->pointerElement()->isArray()
		    && (from->pointerElement()->arrayElement()->isVoid() || to->pointerElement()->arrayElement()->isVoid())
		    && (from->isMutablePointer() || not to->isMutablePointer()))
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
				else if(nn == TYPE_LENGTH)
					return Ok(Type::makeLength());
				else if(nn == TYPE_TREE_INLINE)
					return Ok(Type::makeTreeInlineObj());
				else if(nn == TYPE_TREE_BLOCK)
					return Ok(Type::makeTreeBlockObj());
				else if(nn == TYPE_LAYOUT_OBJECT)
					return Ok(Type::makeLayoutObject());
				else if(nn == TYPE_TREE_INLINE_REF)
					return Ok(Type::makeTreeInlineObjRef());
				else if(nn == TYPE_TREE_BLOCK_REF)
					return Ok(Type::makeTreeBlockObjRef());
				else if(nn == TYPE_LAYOUT_OBJECT_REF)
					return Ok(Type::makeLayoutObjectRef());
			}

			auto decl = TRY(this->current()->lookup(name));
			if(decl.size() > 1)
				return ErrMsg(this->loc(), "ambiguous type '{}'", name);

			return Ok(decl[0]->type);
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


		return ErrMsg(this->loc(), "unknown type");
	}


	ErrorOr<const cst::Definition*> Typechecker::getDefinitionForType(const Type* type)
	{
		auto it = m_type_definitions.find(type);
		if(it == m_type_definitions.end())
			return ErrMsg(this->loc(), "no definition for type '{}'", type);
		else
			return Ok(it->second);
	}

	ErrorOr<const DefnTree*> Typechecker::getDefnTreeForType(const Type* type) const
	{
		if(type->isPointer())
			return this->getDefnTreeForType(type->pointerElement());
		else if(type->isOptional())
			return this->getDefnTreeForType(type->optionalElement());
		else if(type->isArray())
			return this->getDefnTreeForType(type->arrayElement());

		auto it = m_type_definitions.find(type);
		if(it != m_type_definitions.end())
		{
			return Ok(it->second->declaration->declaredTree());
		}
		else
		{
			auto n = this->top()->lookupNamespace("builtin");
			assert(n != nullptr);
			return Ok(n);
		}
	}

	ErrorOr<void> Typechecker::addTypeDefinition(const Type* type, const cst::Definition* defn)
	{
		if(m_type_definitions.contains(type))
			return ErrMsg(this->loc(), "type '{}' was already defined", type);

		m_type_definitions[type] = defn;
		return Ok();
	}

	cst::Definition* Typechecker::addBuiltinDefinition(std::unique_ptr<cst::Definition> defn)
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
		return util::Defer<>([this]() { this->leaveFunctionWithReturnType(); });
	}

	void Typechecker::leaveFunctionWithReturnType()
	{
		assert(not m_expected_return_types.empty());
		m_expected_return_types.pop_back();
	}


	[[nodiscard]] util::Defer<> Typechecker::enterLoopBody()
	{
		m_loop_body_nesting++;
		return util::Defer([this]() { m_loop_body_nesting--; });
	}

	bool Typechecker::isCurrentlyInLoopBody() const
	{
		return m_loop_body_nesting > 0;
	}
}
