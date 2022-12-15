// value.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>
#include <vector>

#include "type.h"
#include "tree.h"

namespace sap::tree
{
	struct InlineObject;
}

namespace sap::interp
{
	struct Value
	{
		const Type* type() const { return m_type; }

		bool getBool() const
		{
			assert(m_kind == KIND_BOOL);
			return v_bool;
		}

		uint32_t getChar() const
		{
			assert(m_kind == KIND_CHAR);
			return v_char;
		}

		double getDouble() const
		{
			assert(m_kind == KIND_FLOATING);
			return v_floating;
		}

		int64_t getInteger() const
		{
			assert(m_kind == KIND_INTEGER);
			return v_integer;
		}

		using FnType = std::optional<Value> (*)(Interpreter*, const std::vector<Value>&);
		FnType getFunction() const
		{
			assert(m_kind == KIND_FUNCTION);
			return v_function;
		}

		tree::InlineObject* getTreeInlineObj() const
		{
			assert(m_kind == KIND_TREE_INLINE_OBJ);
			return v_inline_obj.get();
		}

		std::unique_ptr<tree::InlineObject> takeTreeInlineObj() &&
		{
			assert(m_kind == KIND_TREE_INLINE_OBJ);
			return std::move(v_inline_obj);
		}


		std::string toString() const
		{
			switch(m_kind)
			{
				case KIND_CHAR:
					return "CHAR???";
				case KIND_BOOL:
					return v_bool ? "true" : "false";
				case KIND_INTEGER:
					return std::to_string(v_integer);
				case KIND_FLOATING:
					return std::to_string(v_floating);
				case KIND_FUNCTION:
					return "function";
				case KIND_TREE_INLINE_OBJ:
					return "tree_inline_obj";
				default:
					return "?????";
			}
		}

		bool isBool() const { return m_kind == KIND_BOOL; }
		bool isChar() const { return m_kind == KIND_CHAR; }
		bool isDouble() const { return m_kind == KIND_FLOATING; }
		bool isInteger() const { return m_kind == KIND_INTEGER; }
		bool isFunction() const { return m_kind == KIND_FUNCTION; }
		bool isTreeInlineObj() const { return m_kind == KIND_TREE_INLINE_OBJ; }

		Value() : m_type(Type::makeVoid()), m_kind(KIND_NONE) { }

		~Value() { this->destroy(); }

		Value(const Value&) = delete;
		Value& operator=(const Value&) = delete;

		Value(Value&& val) : m_type(val.m_type), m_kind(val.m_kind)
		{
			// aoeu
			this->steal_from(std::move(val));
		}

		Value& operator=(Value&& val)
		{
			if(this == &val)
				return *this;

			this->destroy();
			this->steal_from(std::move(val));
			return *this;
		}

		enum Kind
		{
			KIND_NONE = 0,
			KIND_BOOL = 1,
			KIND_CHAR = 2,
			KIND_INTEGER = 3,
			KIND_FLOATING = 4,
			KIND_FUNCTION = 5,

			KIND_TREE_INLINE_OBJ = 6,
		};


		static Value boolean(bool value)
		{
			auto ret = Value(Type::makeBool(), KIND_BOOL);
			ret.v_bool = value;
			return ret;
		}

		static Value integer(int64_t num)
		{
			auto ret = Value(Type::makeNumber(), KIND_INTEGER);
			ret.v_integer = num;
			return ret;
		}

		static Value decimal(double num)
		{
			auto ret = Value(Type::makeNumber(), KIND_FLOATING);
			ret.v_floating = num;
			return ret;
		}

		static Value character(uint32_t ch)
		{
			auto ret = Value(Type::makeChar(), KIND_CHAR);
			ret.v_char = ch;
			return ret;
		}

		static Value function(const FunctionType* fn_type, FnType fn)
		{
			auto ret = Value(fn_type, KIND_FUNCTION);
			ret.v_function = fn;
			return ret;
		}

		static Value treeInlineObject(std::unique_ptr<tree::InlineObject> obj)
		{
			auto ret = Value(Type::makeTreeInlineObj(), KIND_TREE_INLINE_OBJ);
			new(&ret.v_inline_obj) decltype(ret.v_inline_obj)(std::move(obj));
			return ret;
		}

	private:
		Value(const Type* type, Kind kind) : m_type(type), m_kind(kind) { }

		void destroy()
		{
			if(m_kind == KIND_TREE_INLINE_OBJ)
				v_inline_obj.~decltype(v_inline_obj)();
		}

		void steal_from(Value&& val)
		{
			switch(val.m_kind)
			{
				case KIND_NONE:
					break;
				case KIND_BOOL:
					v_bool = std::move(val.v_bool);
					break;
				case KIND_CHAR:
					v_char = std::move(val.v_char);
					break;
				case KIND_INTEGER:
					v_integer = std::move(val.v_integer);
					break;
				case KIND_FLOATING:
					v_floating = std::move(val.v_floating);
					break;
				case KIND_FUNCTION:
					v_function = std::move(val.v_function);
					break;
				case KIND_TREE_INLINE_OBJ:
					new(&v_inline_obj) decltype(v_inline_obj)(std::move(val.v_inline_obj));
					break;
			}

			m_kind = val.m_kind;
			m_type = val.m_type;
		}


		const Type* m_type;
		Kind m_kind = KIND_NONE;

		union
		{
			bool v_bool;
			uint32_t v_char;
			int64_t v_integer;
			double v_floating;

			FnType v_function;

			std::unique_ptr<tree::InlineObject> v_inline_obj;
		};
	};
}
