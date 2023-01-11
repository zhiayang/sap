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
		{
			return "any";
		}
		else if(this->isVoid())
		{
			return "void";
		}
		else if(this->isInteger())
		{
			return "int";
		}
		else if(this->isFloating())
		{
			return "float";
		}
		else if(this->isBool())
		{
			return "bool";
		}
		else if(this->isChar())
		{
			return "char";
		}
		else if(this->isNullPtr())
		{
			return "null";
		}
		else if(this->isTreeInlineObj())
		{
			return "Inline";
		}
		else
		{
			return "?????";
		}
	}

	bool Type::sameAs(const Type* other) const
	{
		if(m_kind != other->m_kind)
			return false;

		// note: if the kind is a "primitive", we know that we can just compare m_kind.
		// otherwise, we should call the virtual function. However, we are a primitive; this means
		// that the other guy must be the non-primitive, so call their sameAs() instead.
		if(util::is_one_of(m_kind, KIND_VOID, KIND_ANY, KIND_BOOL, KIND_CHAR, KIND_INTEGER, KIND_FLOATING, KIND_TREE_INLINE_OBJ,
		       KIND_NULLPTR))
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

	const Type* Type::makeBool()
	{
		static Type* bool_type = new Type(KIND_BOOL);
		return bool_type;
	}

	const Type* Type::makeChar()
	{
		static Type* char_type = new Type(KIND_CHAR);
		return char_type;
	}

	const Type* Type::makeNullPtr()
	{
		static Type* nullptr_type = new Type(KIND_NULLPTR);
		return nullptr_type;
	}

	const Type* Type::makeInteger()
	{
		static Type* number_type = new Type(KIND_INTEGER);
		return number_type;
	}

	const Type* Type::makeFloating()
	{
		static Type* number_type = new Type(KIND_FLOATING);
		return number_type;
	}

	const Type* Type::makeString()
	{
		static const Type* arr_type = Type::makeArray(makeChar());
		return arr_type;
	}

	const Type* Type::makeTreeInlineObj()
	{
		static Type* obj_type = new Type(KIND_TREE_INLINE_OBJ);
		return obj_type;
	}

	const Type* Type::makeTreeBlockObj()
	{
		static Type* obj_type = new Type(KIND_TREE_BLOCK_OBJ);
		return obj_type;
	}

	const PointerType* Type::makePointer(const Type* element_type, bool is_mutable)
	{
		return get_or_add_type(new PointerType(element_type, is_mutable));
	}

	const OptionalType* Type::makeOptional(const Type* element_type)
	{
		return get_or_add_type(new OptionalType(element_type));
	}

	const FunctionType* Type::makeFunction(std::vector<const Type*> params, const Type* return_type)
	{
		return get_or_add_type(new FunctionType(std::move(params), return_type));
	}

	const ArrayType* Type::makeArray(const Type* elem, bool is_variadic)
	{
		return get_or_add_type(new ArrayType(elem, is_variadic));
	}

	const StructType* Type::makeStruct(const std::string& name, const std::vector<std::pair<std::string, const Type*>>& foo)
	{
		std::vector<StructType::Field> fields {};
		for(auto& [n, t] : foo)
			fields.push_back(StructType::Field { .name = n, .type = t });

		return get_or_add_type(new StructType(name, std::move(fields)));
	}

	const Type* Type::pointerElement() const
	{
		assert(this->isPointer());
		return this->toPointer()->elementType();
	}

	const Type* Type::optionalElement() const
	{
		assert(this->isOptional());
		return this->toOptional()->elementType();
	}

	bool Type::isMutablePointer() const
	{
		return this->isPointer() && this->toPointer()->isMutable();
	}

	const FunctionType* Type::toFunction() const
	{
		assert(this->isFunction());
		return static_cast<const FunctionType*>(this);
	}

	const OptionalType* Type::toOptional() const
	{
		assert(this->isOptional());
		return static_cast<const OptionalType*>(this);
	}

	const PointerType* Type::toPointer() const
	{
		assert(this->isPointer());
		return static_cast<const PointerType*>(this);
	}

	const ArrayType* Type::toArray() const
	{
		assert(this->isArray());
		return static_cast<const ArrayType*>(this);
	}

	const StructType* Type::toStruct() const
	{
		assert(this->isStruct());
		return static_cast<const StructType*>(this);
	}
}
