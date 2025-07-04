// type.cpp
// Copyright (c) 2022, yuki
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
		else if(this->isInteger())
			return "int";
		else if(this->isFloating())
			return "float";
		else if(this->isBool())
			return "bool";
		else if(this->isChar())
			return "char";
		else if(this->isNullPtr())
			return "null";
		else if(this->isLength())
			return "Length";
		else if(this->isTreeInlineObj())
			return "Inline";
		else if(this->isTreeBlockObj())
			return "Block";
		else if(this->isLayoutObject())
			return "LayoutObject";
		else if(this->isTreeInlineObjRef())
			return "InlineRef";
		else if(this->isTreeBlockObjRef())
			return "BlockRef";
		else if(this->isLayoutObjectRef())
			return "LayoutObjectRef";
		else
			return "?????";
	}

	bool Type::isString() const
	{
		return this->isArray() && this->toArray()->elementType()->isChar();
	}

	bool Type::isVariadicArray() const
	{
		return this->isArray() && this->toArray()->isVariadic();
	}

	bool Type::isCloneable() const
	{
		if(this->isTreeBlockObj() || this->isTreeInlineObj() || this->isLayoutObject())
			return false;

		if(this->isArray())
			return this->arrayElement()->isCloneable();

		return true;
	}

	bool Type::sameAs(const Type* other) const
	{
		if(m_kind != other->m_kind)
			return false;

		// note: if the kind is a "primitive", we know that we can just compare m_kind.
		// otherwise, we should call the virtual function. However, we are a primitive; this means
		// that the other guy must be the non-primitive, so call their sameAs() instead.
		switch(m_kind)
		{
			case KIND_VOID:
			case KIND_ANY:
			case KIND_BOOL:
			case KIND_CHAR:
			case KIND_INTEGER:
			case KIND_FLOATING:
			case KIND_NULLPTR:
			case KIND_LENGTH:
			case KIND_TREE_INLINE_OBJ:
			case KIND_TREE_BLOCK_OBJ:
			case KIND_LAYOUT_OBJ:
			case KIND_TREE_INLINE_OBJ_REF:
			case KIND_TREE_BLOCK_OBJ_REF:
			case KIND_LAYOUT_OBJ_REF: //
				return true;

			default:
				// in theory, this should never end up in a cyclic call.
				return other->sameAs(this);
		}
	}




	struct type_eq
	{
		using is_transparent = void;
		constexpr bool operator()(const std::unique_ptr<Type>& a, const std::unique_ptr<Type>& b) const
		{
			return a->sameAs(b.get());
		}

		constexpr bool operator()(const std::unique_ptr<Type>& a, const Type* b) const { return a->sameAs(b); }

		constexpr bool operator()(const Type* a, const std::unique_ptr<Type>& b) const { return a->sameAs(b.get()); }
	};

	struct type_hash
	{
		using is_transparent = void;
		size_t operator()(const Type* type) const { return util::hasher {}(type->str()); }
		size_t operator()(const std::unique_ptr<Type>& type) const { return util::hasher {}(type->str()); }
	};

	static util::hashset<std::unique_ptr<Type>, type_hash, type_eq> g_type_cache;
	// static std::unordered_set<std::unique_ptr<Type>, type_hash, type_eq> g_type_cache;

	template <typename T>
	static const T* get_or_add_type(T* t)
	{
		if(auto it = g_type_cache.find(static_cast<Type*>(t)); it != g_type_cache.end())
		{
			delete t;
			return static_cast<T*>(it->get());
		}
		else
		{
			return static_cast<T*>(g_type_cache.emplace(t).first->get());
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

	const Type* Type::makeLength()
	{
		static Type* length_type = new Type(KIND_LENGTH);
		return length_type;
	}

	const Type* Type::makeLayoutObject()
	{
		static Type* obj_type = new Type(KIND_LAYOUT_OBJ);
		return obj_type;
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

	const Type* Type::makeLayoutObjectRef()
	{
		static Type* obj_type = new Type(KIND_LAYOUT_OBJ_REF);
		return obj_type;
	}

	const Type* Type::makeTreeBlockObjRef()
	{
		static Type* obj_type = new Type(KIND_TREE_BLOCK_OBJ_REF);
		return obj_type;
	}

	const Type* Type::makeTreeInlineObjRef()
	{
		static Type* obj_type = new Type(KIND_TREE_INLINE_OBJ_REF);
		return obj_type;
	}


	const EnumType* Type::makeEnum(QualifiedId name, const Type* element_type)
	{
		return get_or_add_type(new EnumType(std::move(name), element_type));
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

	const StructType* Type::makeStruct(QualifiedId name, const std::vector<std::pair<std::string, const Type*>>& foo)
	{
		std::vector<StructType::Field> fields {};
		for(auto& [n, t] : foo)
			fields.push_back(StructType::Field { .name = n, .type = t });

		return get_or_add_type(new StructType(std::move(name), std::move(fields)));
	}

	const UnionType* Type::makeUnion(QualifiedId name, //
	    const std::vector<std::pair<std::string, const StructType*>>& foo)
	{
		std::vector<UnionType::Case> cases {};
		for(auto& [n, t] : foo)
			cases.push_back(UnionType::Case { .name = n, .type = t });

		return get_or_add_type(new UnionType(std::move(name), std::move(cases)));
	}

	const Type* Type::arrayElement() const
	{
		assert(this->isArray());
		return this->toArray()->elementType();
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

	const EnumType* Type::toEnum() const
	{
		assert(this->isEnum());
		return static_cast<const EnumType*>(this);
	}

	const UnionType* Type::toUnion() const
	{
		assert(this->isUnion());
		return static_cast<const UnionType*>(this);
	}

	const StructType* Type::toStruct() const
	{
		assert(this->isStruct());
		return static_cast<const StructType*>(this);
	}
}
