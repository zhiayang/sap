// type.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <zpr.h>

#include <cassert>

#include <vector>
#include <string>

namespace sap::interp
{
	struct ArrayType;
	struct FunctionType;

	struct Type
	{
		bool isAny() const { return m_kind == KIND_ANY; }
		bool isVoid() const { return m_kind == KIND_VOID; }
		bool isArray() const { return m_kind == KIND_ARRAY; }
		bool isNumber() const { return m_kind == KIND_NUMBER; }
		bool isString() const { return m_kind == KIND_STRING; }
		bool isFunction() const { return m_kind == KIND_FUNCTION; }
		bool isTreeInlineObj() const { return m_kind == KIND_TREE_INLINE_OBJ; }

		bool isBuiltin() const { return isAny() || isVoid() || isNumber() || isString() || isFunction() || isTreeInlineObj(); }

		// the conversion functions can't be inline because dynamic_cast needs
		// a complete type (and obviously derived types need the complete definition of Type)
		const FunctionType* toFunction() const;
		const ArrayType* toArray() const;

		virtual std::string str() const;
		virtual bool sameAs(const Type* other) const;

		static const Type* makeAny();
		static const Type* makeVoid();
		static const Type* makeNumber();
		static const Type* makeString();
		static const Type* makeTreeInlineObj();

		static const FunctionType* makeFunction(std::vector<const Type*> param_types, const Type* return_type);
		static const ArrayType* makeArray(const Type* element_type, bool is_variadic = false);

	protected:
		static constexpr int KIND_ANY = 0;
		static constexpr int KIND_VOID = 1;
		static constexpr int KIND_NUMBER = 2;
		static constexpr int KIND_STRING = 3;
		static constexpr int KIND_FUNCTION = 4;
		static constexpr int KIND_TREE_INLINE_OBJ = 5;
		static constexpr int KIND_ARRAY = 6;

		int m_kind;

		Type(int kind) : m_kind(kind) { }
		virtual ~Type();
	};

	struct FunctionType : Type
	{
		virtual std::string str() const override;
		virtual bool sameAs(const Type* other) const override;

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
