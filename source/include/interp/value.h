// value.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <numeric>
#include <sstream> // for operator<<, stringstream, basic_ostream

#include "type.h" // for Type, FunctionType, ArrayType
#include "util.h" // for u32StringFromUtf8

#include "sap/units.h"
#include "interp/basedefs.h" // for InlineObject

namespace sap::tree
{
	struct BlockObject;
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
		using FnType = std::optional<Value> (*)(Interpreter*, const std::vector<Value>&);
		using InlineObjects = std::vector<std::unique_ptr<tree::InlineObject>>;

		const Type* type() const;

		bool getBool() const;
		char32_t getChar() const;
		double getFloating() const;
		int64_t getInteger() const;
		FnType getFunction() const;
		DynLength getLength() const;

		const InlineObjects& getTreeInlineObj() const;
		InlineObjects takeTreeInlineObj() &&;

		const tree::BlockObject& getTreeBlockObj() const;
		std::unique_ptr<tree::BlockObject> takeTreeBlockObj() &&;

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

		tree::BlockObject* getTreeBlockObjectRef() const;
		layout::LayoutObject* getLayoutObjectRef() const;

		std::vector<tree::InlineObject*> takeTreeInlineObjectRef() &&;
		const std::vector<tree::InlineObject*>& getTreeInlineObjectRef() const;




		std::optional<const Value*> getOptional() const;
		std::optional<Value*> getOptional();
		std::optional<Value> takeOptional() &&;
		bool haveOptionalValue() const;

		std::u32string toString() const;

		bool isBool() const;
		bool isChar() const;
		bool isEnum() const;
		bool isArray() const;
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
		static Value function(const FunctionType* fn_type, FnType fn);
		static Value string(const std::u32string& str);
		static Value array(const Type* elm, std::vector<Value> arr, bool variadic = false);
		static Value treeInlineObject(InlineObjects obj);
		static Value treeBlockObject(std::unique_ptr<tree::BlockObject> obj);
		static Value layoutObject(std::unique_ptr<layout::LayoutObject> obj);
		static Value structure(const StructType* ty, std::vector<Value> fields);
		static Value pointer(const Type* elm_type, const Value* value);
		static Value mutablePointer(const Type* elm_type, Value* value);
		static Value optional(const Type* type, std::optional<Value> value);
		Value clone() const;

		static Value treeInlineObjectRef(std::vector<tree::InlineObject*> pointers);
		static Value treeBlockObjectRef(tree::BlockObject* obj);
		static Value layoutObjectRef(layout::LayoutObject* obj);

		template <typename T>
		T get() const;

	private:
		explicit Value(const Type* type) : m_type(type), m_moved_from(false) { }

		void destroy();
		void steal_from(Value&& val);
		void ensure_not_moved_from() const;

		static InlineObjects clone_tios(const InlineObjects& from);
		static std::unique_ptr<tree::BlockObject> clone_tbos(const tree::BlockObject& from);

		const Type* m_type;
		bool m_moved_from = false;

		union
		{
			bool v_bool;
			char32_t v_char;
			int64_t v_integer;
			double v_floating;

			FnType v_function;

			InlineObjects v_inline_obj;
			std::unique_ptr<tree::BlockObject> v_block_obj;
			std::unique_ptr<layout::LayoutObject> v_layout_obj;
			std::vector<Value> v_array;
			const Value* v_pointer;
			DynLength v_length;

			// these things need special handling
			tree::BlockObject* v_block_obj_ref;
			layout::LayoutObject* v_layout_obj_ref;
			std::vector<tree::InlineObject*> v_inline_obj_ref;
		};
	};
}
