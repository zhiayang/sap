// value.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <numeric>
#include <sstream> // for operator<<, stringstream, basic_ostream

#include "type.h" // for Type, FunctionType, ArrayType
#include "util.h" // for u32StringFromUtf8

#include "interp/basedefs.h" // for InlineObject

namespace sap::tree
{
	struct InlineObject;
}

namespace sap::interp
{
	struct Interpreter;

	struct Value
	{
		using InlineObjects = std::vector<std::unique_ptr<tree::InlineObject>>;

		const Type* type() const { return m_type; }

		bool getBool() const
		{
			assert(this->isBool());
			return v_bool;
		}

		char32_t getChar() const
		{
			assert(this->isChar());
			return v_char;
		}

		double getFloating() const
		{
			assert(this->isFloating());
			return v_floating;
		}

		int64_t getInteger() const
		{
			assert(this->isInteger());
			return v_integer;
		}

		using FnType = std::optional<Value> (*)(Interpreter*, const std::vector<Value>&);
		FnType getFunction() const
		{
			assert(this->isFunction());
			return v_function;
		}

		const InlineObjects& getTreeInlineObj() const
		{
			assert(this->isTreeInlineObj());
			return v_inline_obj;
		}

		InlineObjects takeTreeInlineObj() &&
		{
			assert(this->isTreeInlineObj());
			return std::move(v_inline_obj);
		}


		const std::vector<Value>& getArray() const
		{
			assert(this->isArray());
			return v_array;
		}

		std::vector<Value> takeArray() &&
		{
			assert(this->isArray());
			return std::move(v_array);
		}

		Value& getStructField(size_t idx)
		{
			assert(this->isStruct());
			return v_array[idx];
		}

		const Value& getStructField(size_t idx) const
		{
			assert(this->isStruct());
			return v_array[idx];
		}

		std::vector<Value> takeStructFields() &&
		{
			assert(this->isStruct());
			return std::move(v_array);
		}

		const std::vector<Value>& getStructFields() const
		{
			assert(this->isStruct());
			return v_array;
		}

		const Value* getPointer() const
		{
			assert(this->isPointer());
			return v_pointer;
		}

		Value* getMutablePointer() const
		{
			assert(m_type->isMutablePointer());
			return const_cast<Value*>(v_pointer);
		}

		std::optional<const Value*> getOptional() const
		{
			assert(m_type->isOptional());
			if(v_array.empty())
				return std::nullopt;

			return &v_array[0];
		}

		std::optional<Value*> getOptional()
		{
			assert(m_type->isOptional());
			if(v_array.empty())
				return std::nullopt;

			return &v_array[0];
		}

		std::optional<Value> takeOptional() &&
		{
			assert(m_type->isOptional());
			if(v_array.empty())
				return std::nullopt;

			auto ret = std::move(v_array[0]);
			v_array.clear();
			return ret;
		}

		bool haveOptionalValue() const
		{
			assert(m_type->isOptional());
			return v_array.size() > 0;
		}



		std::u32string toString() const
		{
			if(this->isChar())
			{
				return &v_char;
			}
			else if(this->isBool())
			{
				return v_bool ? U"true" : U"false";
			}
			else if(this->isInteger())
			{
				return unicode::u32StringFromUtf8(std::to_string(v_integer));
			}
			else if(this->isFloating())
			{
				return unicode::u32StringFromUtf8(std::to_string(v_floating));
			}
			else if(this->isFunction())
			{
				return U"function";
			}
			else if(this->isTreeInlineObj())
			{
				return U"tree_inline_obj";
			}
			else if(this->isOptional())
			{
				if(v_array.empty())
					return U"empty";
				else
					return v_array[0].toString();
			}
			else if(this->isArray())
			{
				if(m_type == Type::makeString())
				{
					std::u32string ret {};
					ret.reserve(v_array.size());

					for(size_t i = 0; i < v_array.size(); i++)
						ret += v_array[i].getChar();

					return ret;
				}
				else
				{
					std::u32string ret {};
					for(size_t i = 0; i < v_array.size(); i++)
					{
						if(i != 0)
							ret += U", ";
						ret += v_array[i].toString();
					}

					ret += U"]";
					return ret;
				}
			}
			else
			{
				return U"?????";
			}
		}

		bool isBool() const { return m_type->isBool(); }
		bool isChar() const { return m_type->isChar(); }
		bool isArray() const { return m_type->isArray(); }
		bool isStruct() const { return m_type->isStruct(); }
		bool isInteger() const { return m_type->isInteger(); }
		bool isPointer() const { return m_type->isPointer(); }
		bool isFloating() const { return m_type->isFloating(); }
		bool isFunction() const { return m_type->isFunction(); }
		bool isOptional() const { return m_type->isOptional(); }
		bool isTreeInlineObj() const { return m_type->isTreeInlineObj(); }

		bool isPrintable() const { return isBool() || isChar() || isArray() || isInteger() || isFloating(); }

		Value() : m_type(Type::makeVoid()) { }
		~Value() { this->destroy(); }

		Value(const Value&) = delete;
		Value& operator=(const Value&) = delete;

		Value(Value&& val) : m_type(val.m_type) { this->steal_from(std::move(val)); }

		Value& operator=(Value&& val)
		{
			if(this == &val)
				return *this;

			this->destroy();
			this->steal_from(std::move(val));
			return *this;
		}


		static Value boolean(bool value)
		{
			auto ret = Value(Type::makeBool());
			ret.v_bool = value;
			return ret;
		}

		static Value integer(int64_t num)
		{
			auto ret = Value(Type::makeInteger());
			ret.v_integer = num;
			return ret;
		}

		static Value floating(double num)
		{
			auto ret = Value(Type::makeFloating());
			ret.v_floating = num;
			return ret;
		}

		static Value character(char32_t ch)
		{
			auto ret = Value(Type::makeChar());
			ret.v_char = ch;
			return ret;
		}

		static Value function(const FunctionType* fn_type, FnType fn)
		{
			auto ret = Value(fn_type);
			ret.v_function = fn;
			return ret;
		}

		static Value string(const std::u32string& str)
		{
			auto ret = Value(Type::makeString());
			new(&ret.v_array) decltype(ret.v_array)();
			ret.v_array.resize(str.size());

			for(size_t i = 0; i < str.size(); i++)
				ret.v_array[i] = Value::character(str[i]);

			return ret;
		}

		static Value array(const Type* elm, std::vector<Value> arr)
		{
			auto ret = Value(Type::makeArray(elm));
			new(&ret.v_array) decltype(ret.v_array)();
			ret.v_array.resize(arr.size());

			for(size_t i = 0; i < arr.size(); i++)
				ret.v_array[i] = std::move(arr[i]);

			return ret;
		}

		static Value treeInlineObject(InlineObjects obj)
		{
			auto ret = Value(Type::makeTreeInlineObj());
			new(&ret.v_inline_obj) decltype(ret.v_inline_obj)(std::move(obj));
			return ret;
		}

		static Value structure(const StructType* ty, std::vector<Value> fields)
		{
			auto ret = Value(ty);
			new(&ret.v_array) decltype(ret.v_array)(std::move(fields));
			return ret;
		}

		static Value nullPointer()
		{
			auto ret = Value(Type::makeNullPtr());
			ret.v_pointer = nullptr;
			return ret;
		}

		static Value pointer(const Type* elm_type, const Value* value)
		{
			auto ret = Value(elm_type->pointerTo());
			ret.v_pointer = value;
			return ret;
		}

		static Value mutablePointer(const Type* elm_type, Value* value)
		{
			auto ret = Value(elm_type->mutablePointerTo());
			ret.v_pointer = value;
			return ret;
		}

		static Value optional(const Type* type, std::optional<Value> value)
		{
			auto ret = Value(type->optionalOf());
			new(&ret.v_array) decltype(v_array)();
			if(value.has_value())
				ret.v_array.push_back(std::move(*value));

			return ret;
		}


		Value clone() const
		{
			auto val = Value(m_type);
			if(m_type->isBool())
			{
				val.v_bool = v_bool;
			}
			else if(m_type->isChar())
			{
				val.v_char = v_char;
			}
			else if(m_type->isInteger())
			{
				val.v_integer = v_integer;
			}
			else if(m_type->isFloating())
			{
				val.v_floating = v_floating;
			}
			else if(m_type->isPointer() || m_type->isNullPtr())
			{
				val.v_pointer = v_pointer;
			}
			else if(m_type->isFunction())
			{
				val.v_function = v_function;
			}
			else if(m_type->isTreeInlineObj())
			{
				new(&val.v_inline_obj) decltype(v_inline_obj)();
				val.v_inline_obj = Value::clone_tios(v_inline_obj);
			}
			else if(m_type->isArray() || m_type->isStruct() || m_type->isOptional())
			{
				new(&val.v_array) decltype(v_array)();
				for(auto& e : v_array)
					val.v_array.push_back(e.clone());
			}
			else
			{
				zpr::println("weird type: {}", m_type);
				assert(false && "unreachable!");
			}

			return val;
		}

	private:
		explicit Value(const Type* type) : m_type(type) { }

		void destroy()
		{
			if(m_type->isTreeInlineObj())
				v_inline_obj.~decltype(v_inline_obj)();
			else if(m_type->isArray() || m_type->isStruct() || m_type->isOptional())
				v_array.~decltype(v_array)();
		}

		void steal_from(Value&& val)
		{
			m_type = val.m_type;
			if(m_type->isBool())
				v_bool = std::move(val.v_bool);
			else if(m_type->isChar())
				v_char = std::move(val.v_char);
			else if(m_type->isInteger())
				v_integer = std::move(val.v_integer);
			else if(m_type->isFloating())
				v_floating = std::move(val.v_floating);
			else if(m_type->isFunction())
				v_function = std::move(val.v_function);
			else if(m_type->isPointer() || m_type->isNullPtr())
				v_pointer = std::move(val.v_pointer);
			else if(m_type->isTreeInlineObj())
				new(&v_inline_obj) decltype(v_inline_obj)(std::move(val.v_inline_obj));
			else if(m_type->isArray() || m_type->isStruct() || m_type->isOptional())
				new(&v_array) decltype(v_array)(std::move(val.v_array));
			else
				assert(false && "unreachable!");
		}

		static InlineObjects clone_tios(const InlineObjects& from);


		const Type* m_type;

		union
		{
			bool v_bool;
			char32_t v_char;
			int64_t v_integer;
			double v_floating;

			FnType v_function;

			InlineObjects v_inline_obj;
			std::vector<Value> v_array;
			const Value* v_pointer;
		};
	};
}
