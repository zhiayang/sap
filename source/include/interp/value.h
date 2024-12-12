// value.h
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <functional>

#include "type.h"
#include "sap/units.h"

namespace sap::tree
{
	struct BlockObject;

	struct InlineSpan;
	struct InlineObject;
}

namespace sap::layout
{
	struct LayoutObject;
}

namespace sap::interp
{
	struct Interpreter;

	struct Value
	{
		using FnType = std::shared_ptr<std::function<std::optional<Value>(Interpreter*, std::vector<Value>&)>>;

		const Type* type() const;

		bool getBool() const;
		char32_t getChar() const;
		double getFloating() const;
		int64_t getInteger() const;
		FnType getFunction() const;
		DynLength getLength() const;

		const tree::InlineSpan& getTreeInlineObj() const;
		zst::SharedPtr<tree::InlineSpan> takeTreeInlineObj() &&;

		const tree::BlockObject& getTreeBlockObj() const;
		zst::SharedPtr<tree::BlockObject> takeTreeBlockObj() &&;

		const layout::LayoutObject& getLayoutObject() const;
		std::unique_ptr<layout::LayoutObject> takeLayoutObject() &&;

		std::string getUtf8String() const;
		std::u32string getUtf32String() const;

		const std::vector<Value>& getArray() const;
		std::vector<Value> takeArray() &&;

		const Value& getEnumerator() const;
		Value takeEnumerator() &&;

		Value& getStructField(size_t idx);
		const Value& getStructField(size_t idx) const;

		Value& getStructField(zst::str_view name);
		const Value& getStructField(zst::str_view name) const;

		std::vector<Value> takeStructFields() &&;
		const std::vector<Value>& getStructFields() const;

		const Value* getPointer() const;
		Value* getMutablePointer() const;

		tree::InlineSpan* getTreeInlineObjectRef() const;
		tree::BlockObject* getTreeBlockObjectRef() const;
		layout::LayoutObject* getLayoutObjectRef() const;

		const Value& getUnionUnderlyingStruct() const;
		Value& getUnionUnderlyingStruct();
		Value takeUnionUnderlyingStruct() &&;


		std::optional<const Value*> getOptional() const;
		std::optional<Value*> getOptional();
		std::optional<Value> takeOptional() &&;
		bool haveOptionalValue() const;

		bool hasGenerator() const;

		std::u32string toString() const;

		bool isNull() const;
		bool isBool() const;
		bool isChar() const;
		bool isEnum() const;
		bool isArray() const;
		bool isUnion() const;
		bool isString() const;
		bool isStruct() const;
		bool isLength() const;
		bool isInteger() const;
		bool isFloating() const;
		bool isFunction() const;
		bool isOptional() const;
		bool isLayoutObject() const;
		bool isTreeBlockObj() const;
		bool isTreeInlineObj() const;

		bool isPointer() const;
		bool isTreeInlineObjRef() const;
		bool isTreeBlockObjRef() const;
		bool isLayoutObjectRef() const;

		bool isPrintable() const;

		Value(const Value&) = delete;
		Value& operator=(const Value&) = delete;

		Value();
		~Value();

		Value(Value&& val);
		Value& operator=(Value&& val);

		static Value nullPointer();
		static Value length(DynLength len);
		static Value boolean(bool value);
		static Value integer(int64_t num);
		static Value floating(double num);
		static Value character(char32_t ch);
		static Value enumerator(const EnumType* type, Value value);
		static Value unionVariant(const UnionType* type, size_t case_idx, Value case_value_as_struct);
		static Value function(const FunctionType* fn_type, typename FnType::element_type fn);
		static Value string(const std::string& str);
		static Value string(const std::u32string& str);
		static Value array(const Type* elm, std::vector<Value> arr, bool variadic = false);
		static Value treeInlineObject(zst::SharedPtr<tree::InlineSpan> obj);
		static Value treeBlockObject(zst::SharedPtr<tree::BlockObject> obj);
		static Value layoutObject(std::unique_ptr<layout::LayoutObject> obj);
		static Value structure(const StructType* ty, std::vector<Value> fields);
		static Value pointer(const Type* elm_type, const Value* value);
		static Value mutablePointer(const Type* elm_type, Value* value);
		static Value optional(const Type* type, std::optional<Value> value);

		static Value fromGenerator(const Type* type, std::function<Value()> func);

		static Value treeInlineObjectRef(tree::InlineSpan* obj);
		static Value treeBlockObjectRef(tree::BlockObject* obj);
		static Value layoutObjectRef(layout::LayoutObject* obj);

		template <typename T>
		T get() const;

		Value clone() const;

		size_t getUnionVariantIndex() const;

	private:
		explicit Value(const Type* type) : m_type(type), m_moved_from(false) { }

		void destroy();
		void steal_from(Value&& val);
		void ensure_not_moved_from() const;

		const Type* m_type;
		bool m_moved_from = false;
		mutable std::function<Value(void)> m_gen_func {};

		mutable size_t m_union_case_idx = 0;

		union
		{
			mutable bool v_bool;
			mutable char32_t v_char;
			mutable int64_t v_integer;
			mutable double v_floating;

			mutable FnType v_function;

			zst::SharedPtr<tree::InlineSpan> v_inline_obj;
			zst::SharedPtr<tree::BlockObject> v_block_obj;
			std::unique_ptr<layout::LayoutObject> v_layout_obj;
			mutable std::vector<Value> v_array;
			mutable const Value* v_pointer;
			mutable DynLength v_length;

			// these things need special handling
			mutable tree::InlineSpan* v_inline_obj_ref;
			mutable tree::BlockObject* v_block_obj_ref;
			mutable layout::LayoutObject* v_layout_obj_ref;
		};
	};
}
