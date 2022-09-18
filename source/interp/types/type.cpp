// type.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/type.h"

namespace sap::interp
{
	Type::~Type()
	{
	}

	std::string Type::str() const
	{
		if(this->isAny())
			return "any";
		else if(this->isVoid())
			return "void";
		else if(this->isNumber())
			return "num";
		else if(this->isString())
			return "str";
		else if(this->isTreeInlineObj())
			return "InlineObj";
		else
			return "??";
	}

	bool Type::sameAs(const Type* other) const
	{
		if(m_kind != other->m_kind)
			return false;

		// note: if the kind is a "primitive", we know that we can just compare m_kind.
		// otherwise, we should call the virtual function. However, we are a primitive; this means
		// that the other guy must be the non-primitive, so call their sameAs() instead.
		if(m_kind == KIND_VOID || m_kind == KIND_NUMBER || m_kind == KIND_STRING || m_kind == KIND_ANY ||
			m_kind == KIND_TREE_INLINE_OBJ)
		{
			return true;
		}

		// in theory, this should never end up in a cyclic call.
		return other->sameAs(this);
	}




	struct type_eq
	{
		constexpr bool operator()(const Type* a, const Type* b) const { return a->sameAs(b); }
	};

	struct type_hash
	{
		size_t operator()(const Type* type) const { return std::hash<std::string> {}(type->str()); }
	};

	std::unordered_set<Type*, type_hash, type_eq> g_type_cache;

	template <typename T>
	static const T* get_or_add_type(T* t)
	{
		if(auto it = g_type_cache.find(t); it != g_type_cache.end())
		{
			delete t;
			return dynamic_cast<T*>(*it);
		}
		else
		{
			g_type_cache.insert(t);
			return t;
		}
	}


	const Type* Type::makeAny()
	{
		static Type* any_type = new Type(KIND_ANY);
		return any_type;
	}

	const Type* Type::makeVoid()
	{
		static Type* void_type = new Type(KIND_VOID);
		return void_type;
	}

	const Type* Type::makeNumber()
	{
		static Type* number_type = new Type(KIND_NUMBER);
		return number_type;
	}

	const Type* Type::makeString()
	{
		static Type* string_type = new Type(KIND_STRING);
		return string_type;
	}

	const Type* Type::makeTreeInlineObj()
	{
		static Type* obj_type = new Type(KIND_TREE_INLINE_OBJ);
		return obj_type;
	}

	const FunctionType* Type::makeFunction(std::vector<const Type*> params, const Type* return_type)
	{
		return get_or_add_type(new FunctionType(std::move(params), return_type));
	}

	const ArrayType* Type::makeArray(const Type* elem, bool is_variadic)
	{
		return get_or_add_type(new ArrayType(elem, is_variadic));
	}






	const FunctionType* Type::toFunction() const
	{
		assert(isFunction());
		return dynamic_cast<const FunctionType*>(this);
	}

	const ArrayType* Type::toArray() const
	{
		assert(isArray());
		return dynamic_cast<const ArrayType*>(this);
	}
}
