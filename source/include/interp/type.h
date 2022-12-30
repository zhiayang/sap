// type.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"

namespace sap::interp
{
	struct ArrayType;
	struct StructType;
	struct FunctionType;

	struct Type
	{
		bool isAny() const { return m_kind == KIND_ANY; }
		bool isVoid() const { return m_kind == KIND_VOID; }
		bool isBool() const { return m_kind == KIND_BOOL; }
		bool isChar() const { return m_kind == KIND_CHAR; }
		bool isArray() const { return m_kind == KIND_ARRAY; }
		bool isStruct() const { return m_kind == KIND_STRUCT; }
		bool isInteger() const { return m_kind == KIND_INTEGER; }
		bool isFloating() const { return m_kind == KIND_FLOATING; }
		bool isFunction() const { return m_kind == KIND_FUNCTION; }
		bool isTreeInlineObj() const { return m_kind == KIND_TREE_INLINE_OBJ; }

		bool isBuiltin() const
		{
			return isAny()      //
			    || isVoid()     //
			    || isBool()     //
			    || isChar()     //
			    || isInteger()  //
			    || isFloating() //
			    || isFunction() //
			    || isTreeInlineObj();
		}

		// the conversion functions can't be inline because dynamic_cast needs
		// a complete type (and obviously derived types need the complete definition of Type)
		const FunctionType* toFunction() const;
		const StructType* toStruct() const;
		const ArrayType* toArray() const;

		virtual std::string str() const;
		virtual bool sameAs(const Type* other) const;

		static const Type* makeAny();
		static const Type* makeVoid();
		static const Type* makeBool();
		static const Type* makeChar();
		static const Type* makeString();
		static const Type* makeInteger();
		static const Type* makeFloating();
		static const Type* makeTreeInlineObj();

		static const FunctionType* makeFunction(std::vector<const Type*> param_types, const Type* return_type);
		static const ArrayType* makeArray(const Type* element_type, bool is_variadic = false);
		static const StructType* makeStruct(const std::string& name,
		    const std::vector<std::pair<std::string, const Type*>>& fields);

	protected:
		enum Kind
		{
			KIND_VOID = 0,
			KIND_ANY = 1,
			KIND_BOOL = 2,
			KIND_CHAR = 3,
			KIND_INTEGER = 4,
			KIND_FLOATING = 5,
			KIND_FUNCTION = 6,

			KIND_TREE_INLINE_OBJ = 7,
			KIND_ARRAY = 8,
			KIND_STRUCT = 9,
		};

		Kind m_kind;

		Type(Kind kind) : m_kind(kind) { }
		virtual ~Type();
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

	struct StructType : Type
	{
		struct Field
		{
			std::string name;
			const Type* type;
		};

		virtual std::string str() const override;
		virtual bool sameAs(const Type* other) const override;

		const std::string& name() const { return m_name; }

		bool hasFieldNamed(zst::str_view name) const;
		size_t getFieldIndex(zst::str_view name) const;
		const Type* getFieldNamed(zst::str_view name) const;

		void setFields(std::vector<Field> fields);

		const std::vector<Field>& getFields() const;
		std::vector<const Type*> getFieldTypes() const;

	private:
		StructType(std::string name, std::vector<Field> fields);

		std::string m_name;
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
