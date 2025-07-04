// type.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

#include "util.h"
#include "interp/basedefs.h"

namespace sap::interp
{
	struct EnumType;
	struct ArrayType;
	struct UnionType;
	struct StructType;
	struct PointerType;
	struct OptionalType;
	struct FunctionType;

	struct Type
	{
		bool isAny() const { return m_kind == KIND_ANY; }
		bool isVoid() const { return m_kind == KIND_VOID; }
		bool isBool() const { return m_kind == KIND_BOOL; }
		bool isChar() const { return m_kind == KIND_CHAR; }
		bool isEnum() const { return m_kind == KIND_ENUM; }
		bool isArray() const { return m_kind == KIND_ARRAY; }
		bool isUnion() const { return m_kind == KIND_UNION; }
		bool isStruct() const { return m_kind == KIND_STRUCT; }
		bool isLength() const { return m_kind == KIND_LENGTH; }
		bool isNullPtr() const { return m_kind == KIND_NULLPTR; }
		bool isInteger() const { return m_kind == KIND_INTEGER; }
		bool isPointer() const { return m_kind == KIND_POINTER; }
		bool isOptional() const { return m_kind == KIND_OPTIONAL; }
		bool isFloating() const { return m_kind == KIND_FLOATING; }
		bool isFunction() const { return m_kind == KIND_FUNCTION; }
		bool isLayoutObject() const { return m_kind == KIND_LAYOUT_OBJ; }
		bool isTreeBlockObj() const { return m_kind == KIND_TREE_BLOCK_OBJ; }
		bool isTreeInlineObj() const { return m_kind == KIND_TREE_INLINE_OBJ; }
		bool isLayoutObjectRef() const { return m_kind == KIND_LAYOUT_OBJ_REF; }
		bool isTreeBlockObjRef() const { return m_kind == KIND_TREE_BLOCK_OBJ_REF; }
		bool isTreeInlineObjRef() const { return m_kind == KIND_TREE_INLINE_OBJ_REF; }

		// the conversion functions can't be inline because dynamic_cast needs
		// a complete type (and obviously derived types need the complete definition of Type)
		const FunctionType* toFunction() const;
		const OptionalType* toOptional() const;
		const PointerType* toPointer() const;
		const StructType* toStruct() const;
		const UnionType* toUnion() const;
		const ArrayType* toArray() const;
		const EnumType* toEnum() const;

		bool isCloneable() const;

		virtual std::string str() const;
		virtual bool sameAs(const Type* other) const;

		bool isString() const;
		bool isVariadicArray() const;
		bool isMutablePointer() const;

		const OptionalType* optionalOf() const { return makeOptional(this); }
		const PointerType* pointerTo(bool mut = false) const { return makePointer(this, mut); }
		const PointerType* mutablePointerTo() const { return makePointer(this, true); }

		// convenience functions
		const Type* arrayElement() const;
		const Type* pointerElement() const;
		const Type* optionalElement() const;

		static const Type* makeAny();
		static const Type* makeVoid();
		static const Type* makeBool();
		static const Type* makeChar();
		static const Type* makeString();
		static const Type* makeLength();
		static const Type* makeInteger();
		static const Type* makeNullPtr();
		static const Type* makeFloating();
		static const Type* makeLayoutObject();
		static const Type* makeTreeBlockObj();
		static const Type* makeTreeInlineObj();
		static const Type* makeLayoutObjectRef();
		static const Type* makeTreeBlockObjRef();
		static const Type* makeTreeInlineObjRef();

		static const PointerType* makePointer(const Type* element_type, bool is_mutable);
		static const OptionalType* makeOptional(const Type* element_type);

		static const FunctionType* makeFunction(std::vector<const Type*> param_types, const Type* return_type);
		static const ArrayType* makeArray(const Type* element_type, bool is_variadic = false);
		static const StructType* makeStruct(QualifiedId name,
		    const std::vector<std::pair<std::string, const Type*>>& fields);

		static const UnionType* makeUnion(QualifiedId name,
		    const std::vector<std::pair<std::string, const StructType*>>& cases);

		static const EnumType* makeEnum(QualifiedId name, const Type* enumerator_type);

		virtual ~Type();


	protected:
		enum Kind
		{
			KIND_VOID,
			KIND_ANY,
			KIND_BOOL,
			KIND_CHAR,
			KIND_INTEGER,
			KIND_FLOATING,
			KIND_FUNCTION,

			KIND_ENUM,
			KIND_ARRAY,
			KIND_UNION,
			KIND_STRUCT,
			KIND_POINTER,
			KIND_NULLPTR,
			KIND_OPTIONAL,

			KIND_LENGTH,
			KIND_TREE_BLOCK_OBJ,
			KIND_TREE_INLINE_OBJ,
			KIND_LAYOUT_OBJ,

			KIND_TREE_BLOCK_OBJ_REF,
			KIND_TREE_INLINE_OBJ_REF,
			KIND_LAYOUT_OBJ_REF,
		};

		Kind m_kind;

		explicit Type(Kind kind) : m_kind(kind) { }
	};

	struct EnumType : Type
	{
		virtual std::string str() const override;
		virtual bool sameAs(const Type* other) const override;

		const QualifiedId& name() const { return m_name; }
		const Type* elementType() const { return m_element_type; }

	private:
		EnumType(QualifiedId name, const Type* elm) : Type(KIND_ENUM), m_name(std::move(name)), m_element_type(elm) { }

		QualifiedId m_name;
		const Type* m_element_type;

		friend struct Type;
	};

	struct PointerType : Type
	{
		virtual std::string str() const override;
		virtual bool sameAs(const Type* other) const override;

		const Type* elementType() const { return m_element_type; }
		bool isMutable() const { return m_is_mutable; }

	private:
		PointerType(const Type* elm, bool is_mutable)
		    : Type(KIND_POINTER), m_element_type(elm), m_is_mutable(is_mutable)
		{
		}

		const Type* m_element_type;
		bool m_is_mutable;

		friend struct Type;
	};

	struct OptionalType : Type
	{
		virtual std::string str() const override;
		virtual bool sameAs(const Type* other) const override;

		const Type* elementType() const { return m_element_type; }

	private:
		OptionalType(const Type* elm) : Type(KIND_OPTIONAL), m_element_type(elm) { }

		const Type* m_element_type;

		friend struct Type;
	};

	struct FunctionType : Type
	{
		virtual std::string str() const override;
		virtual bool sameAs(const Type* other) const override;

		const Type* returnType() const { return m_return_type; }
		const std::vector<const Type*>& parameterTypes() const { return m_params; }

	private:
		FunctionType(std::vector<const Type*> params, const Type* return_type);

		std::vector<const Type*> m_params;
		const Type* m_return_type;

		friend struct Type;
	};

	struct ArrayType : Type
	{
		virtual std::string str() const override;
		virtual bool sameAs(const Type* other) const override;

		const Type* elementType() const { return m_element_type; }
		bool isVariadic() const { return m_is_variadic; }

	private:
		ArrayType(const Type* elem_type, bool variadic);

		const Type* m_element_type;
		bool m_is_variadic;

		friend struct Type;
	};

	struct UnionType : Type
	{
		// each case is really just a struct type.
		struct Case
		{
			std::string name;
			const StructType* type;
		};

		virtual std::string str() const override;
		virtual bool sameAs(const Type* other) const override;

		const QualifiedId& name() const { return m_name; }

		bool hasCaseNamed(zst::str_view name) const;
		size_t getCaseIndex(zst::str_view name) const;
		const StructType* getCaseNamed(zst::str_view name) const;
		const StructType* getCaseAtIndex(size_t idx) const;

		const std::vector<Case>& getCases() const;
		std::vector<const StructType*> getCaseTypes() const;

		void setCases(std::vector<Case> cases);

	private:
		UnionType(QualifiedId name, std::vector<Case> Cases);

		QualifiedId m_name;
		std::vector<Case> m_cases;
		util::hashmap<std::string, std::pair<size_t, const StructType*>> m_case_map;

		friend struct Type;
	};

	struct StructType : Type
	{
		struct Field
		{
			std::string name;
			const Type* type;
		};

		virtual std::string str() const override;
		virtual bool sameAs(const Type* other) const override;

		const QualifiedId& name() const { return m_name; }

		bool hasFieldNamed(zst::str_view name) const;
		size_t getFieldIndex(zst::str_view name) const;
		const Type* getFieldNamed(zst::str_view name) const;
		const Type* getFieldAtIndex(size_t idx) const;

		void setFields(std::vector<Field> fields);

		const std::vector<Field>& getFields() const;
		std::vector<const Type*> getFieldTypes() const;

	private:
		StructType(QualifiedId name, std::vector<Field> fields);

		QualifiedId m_name;
		std::vector<Field> m_fields;
		util::hashmap<std::string, std::pair<size_t, const Type*>> m_field_map;

		friend struct Type;
	};
}

template <>
struct zpr::print_formatter<const sap::interp::Type*>
{
	template <typename _Cb>
	void print(const sap::interp::Type* type, _Cb&& cb, format_args args)
	{
		detail::print_one(static_cast<_Cb&&>(cb), static_cast<format_args&&>(args), type->str());
	}
};
