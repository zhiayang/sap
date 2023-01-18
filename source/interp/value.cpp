// value.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <utf8proc/utf8proc.h>

#include "tree/image.h"
#include "tree/container.h"
#include "tree/paragraph.h"

#include "interp/value.h"

namespace sap::interp
{
	const Type* Value::type() const
	{
		return m_type;
	}

	bool Value::getBool() const
	{
		this->ensure_not_moved_from();
		assert(this->isBool());
		return v_bool;
	}

	char32_t Value::getChar() const
	{
		this->ensure_not_moved_from();
		assert(this->isChar());
		return v_char;
	}

	double Value::getFloating() const
	{
		this->ensure_not_moved_from();
		assert(this->isFloating());
		return v_floating;
	}

	int64_t Value::getInteger() const
	{
		this->ensure_not_moved_from();
		assert(this->isInteger());
		return v_integer;
	}

	DynLength Value::getLength() const
	{
		this->ensure_not_moved_from();
		assert(this->isLength());
		return v_length;
	}

	auto Value::getFunction() const -> FnType
	{
		this->ensure_not_moved_from();
		assert(this->isFunction());
		return v_function;
	}

	auto Value::getTreeInlineObj() const -> const InlineObjects&
	{
		this->ensure_not_moved_from();
		assert(this->isTreeInlineObj());
		return v_inline_obj;
	}

	auto Value::takeTreeInlineObj() && -> InlineObjects
	{
		this->ensure_not_moved_from();
		assert(this->isTreeInlineObj());
		return std::move(v_inline_obj);
	}

	auto Value::getTreeBlockObj() const -> const tree::BlockObject&
	{
		this->ensure_not_moved_from();
		assert(this->isTreeBlockObj());
		return *v_block_obj;
	}

	auto Value::takeTreeBlockObj() && -> std::unique_ptr<tree::BlockObject>
	{
		this->ensure_not_moved_from();
		assert(this->isTreeBlockObj());
		return std::move(v_block_obj);
	}

	const std::vector<Value>& Value::getArray() const
	{
		this->ensure_not_moved_from();
		assert(this->isArray());
		return v_array;
	}

	std::vector<Value> Value::takeArray() &&
	{
		this->ensure_not_moved_from();
		assert(this->isArray());
		return std::move(v_array);
	}

	const Value& Value::getEnumerator() const
	{
		this->ensure_not_moved_from();
		assert(this->isEnum());
		return v_array[0];
	}

	Value Value::takeEnumerator() &&
	{
		this->ensure_not_moved_from();
		assert(this->isEnum());
		return std::move(v_array[0]);
	}




	std::string Value::getUtf8String() const
	{
		this->ensure_not_moved_from();
		assert(this->isArray());

		std::string ret {};
		ret.reserve(v_array.size());

		for(size_t i = 0; i < v_array.size(); i++)
		{
			uint8_t buf[4] {};
			auto n = utf8proc_encode_char((int32_t) v_array[i].getChar(), &buf[0]);
			ret.append((char*) &buf[0], (size_t) n);
		}

		return ret;
	}

	std::u32string Value::getUtf32String() const
	{
		this->ensure_not_moved_from();
		assert(this->isArray());

		std::u32string ret {};
		ret.reserve(v_array.size());

		for(size_t i = 0; i < v_array.size(); i++)
			ret += v_array[i].getChar();

		return ret;
	}


	Value& Value::getStructField(size_t idx)
	{
		this->ensure_not_moved_from();
		assert(this->isStruct());
		return v_array[idx];
	}

	const Value& Value::getStructField(size_t idx) const
	{
		this->ensure_not_moved_from();
		assert(this->isStruct());
		return v_array[idx];
	}

	Value& Value::getStructField(zst::str_view name)
	{
		this->ensure_not_moved_from();
		assert(this->isStruct());

		return v_array[m_type->toStruct()->getFieldIndex(name)];
	}

	const Value& Value::getStructField(zst::str_view name) const
	{
		this->ensure_not_moved_from();
		assert(this->isStruct());
		return v_array[m_type->toStruct()->getFieldIndex(name)];
	}


	std::vector<Value> Value::takeStructFields() &&
	{
		this->ensure_not_moved_from();
		assert(this->isStruct());
		return std::move(v_array);
	}

	const std::vector<Value>& Value::getStructFields() const
	{
		this->ensure_not_moved_from();
		assert(this->isStruct());
		return v_array;
	}

	const Value* Value::getPointer() const
	{
		this->ensure_not_moved_from();
		assert(this->isPointer());
		return v_pointer;
	}

	Value* Value::getMutablePointer() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isMutablePointer());
		return const_cast<Value*>(v_pointer);
	}

	std::optional<const Value*> Value::getOptional() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isOptional());
		if(v_array.empty())
			return std::nullopt;

		return &v_array[0];
	}

	std::optional<Value*> Value::getOptional()
	{
		this->ensure_not_moved_from();
		assert(m_type->isOptional());
		if(v_array.empty())
			return std::nullopt;

		return &v_array[0];
	}

	std::optional<Value> Value::takeOptional() &&
	{
		this->ensure_not_moved_from();
		assert(m_type->isOptional());
		if(v_array.empty())
			return std::nullopt;

		auto ret = std::move(v_array[0]);
		v_array.clear();
		return ret;
	}

	bool Value::haveOptionalValue() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isOptional());
		return v_array.size() > 0;
	}



	std::u32string Value::toString() const
	{
		this->ensure_not_moved_from();
		if(this->isChar())
		{
			return &v_char;
		}
		else if(this->isBool())
		{
			return v_bool ? U"true" : U"false";
		}
		else if(this->isLength())
		{
			return unicode::u32StringFromUtf8(zpr::sprint("{}{}", v_length.value(), DynLength::unitToString(v_length.unit())));
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
		else if(this->isTreeBlockObj())
		{
			return U"Block{...}";
		}
		else if(this->isTreeInlineObj())
		{
			return U"Inline{...}";
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
				return this->getUtf32String();
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

	bool Value::isBool() const
	{
		return m_type->isBool();
	}
	bool Value::isChar() const
	{
		return m_type->isChar();
	}
	bool Value::isEnum() const
	{
		return m_type->isEnum();
	}
	bool Value::isArray() const
	{
		return m_type->isArray();
	}
	bool Value::isString() const
	{
		return m_type->isArray() && m_type->toArray()->elementType()->isChar();
	}
	bool Value::isStruct() const
	{
		return m_type->isStruct();
	}
	bool Value::isLength() const
	{
		return m_type->isLength();
	}
	bool Value::isInteger() const
	{
		return m_type->isInteger();
	}
	bool Value::isPointer() const
	{
		return m_type->isPointer();
	}
	bool Value::isFloating() const
	{
		return m_type->isFloating();
	}
	bool Value::isFunction() const
	{
		return m_type->isFunction();
	}
	bool Value::isOptional() const
	{
		return m_type->isOptional();
	}
	bool Value::isTreeInlineObj() const
	{
		return m_type->isTreeInlineObj();
	}
	bool Value::isTreeBlockObj() const
	{
		return m_type->isTreeBlockObj();
	}

	bool Value::isPrintable() const
	{
		return isBool() || isChar() || isArray() || isInteger() || isFloating() || isLength();
	}

	Value::Value() : m_type(Type::makeVoid()), m_moved_from(false)
	{
	}

	Value::~Value()
	{
		this->destroy();
	}

	Value::Value(Value&& val) : m_type(val.m_type), m_moved_from(false)
	{
		this->steal_from(std::move(val));
	}

	Value& Value::operator=(Value&& val)
	{
		if(this == &val)
			return *this;

		this->destroy();
		this->steal_from(std::move(val));

		m_moved_from = false;
		return *this;
	}

	void Value::ensure_not_moved_from() const
	{
		// TODO: make this emit location information
		if(m_moved_from)
			sap::internal_error("use of moved-from value!");
	}



	Value Value::length(DynLength len)
	{
		auto ret = Value(Type::makeLength());
		new(&ret.v_length) DynLength(len);
		return ret;
	}

	Value Value::boolean(bool value)
	{
		auto ret = Value(Type::makeBool());
		ret.v_bool = value;
		return ret;
	}

	Value Value::integer(int64_t num)
	{
		auto ret = Value(Type::makeInteger());
		ret.v_integer = num;
		return ret;
	}

	Value Value::floating(double num)
	{
		auto ret = Value(Type::makeFloating());
		ret.v_floating = num;
		return ret;
	}

	Value Value::character(char32_t ch)
	{
		auto ret = Value(Type::makeChar());
		ret.v_char = ch;
		return ret;
	}

	Value Value::function(const FunctionType* fn_type, FnType fn)
	{
		auto ret = Value(fn_type);
		ret.v_function = fn;
		return ret;
	}

	Value Value::string(const std::u32string& str)
	{
		auto ret = Value(Type::makeString());
		new(&ret.v_array) decltype(ret.v_array)();
		ret.v_array.resize(str.size());

		for(size_t i = 0; i < str.size(); i++)
			ret.v_array[i] = Value::character(str[i]);

		return ret;
	}

	Value Value::enumerator(const EnumType* type, Value value)
	{
		auto ret = Value(type);
		new(&ret.v_array) decltype(ret.v_array)();
		ret.v_array.push_back(std::move(value));

		return ret;
	}

	Value Value::array(const Type* elm, std::vector<Value> arr)
	{
		auto ret = Value(Type::makeArray(elm));
		new(&ret.v_array) decltype(ret.v_array)();
		ret.v_array.resize(arr.size());

		for(size_t i = 0; i < arr.size(); i++)
			ret.v_array[i] = std::move(arr[i]);

		return ret;
	}

	Value Value::treeInlineObject(InlineObjects obj)
	{
		auto ret = Value(Type::makeTreeInlineObj());
		new(&ret.v_inline_obj) decltype(ret.v_inline_obj)(std::move(obj));
		return ret;
	}

	Value Value::treeBlockObject(std::unique_ptr<tree::BlockObject> obj)
	{
		auto ret = Value(Type::makeTreeBlockObj());
		new(&ret.v_block_obj) decltype(ret.v_block_obj)(std::move(obj));
		return ret;
	}

	Value Value::structure(const StructType* ty, std::vector<Value> fields)
	{
		auto ret = Value(ty);
		new(&ret.v_array) decltype(ret.v_array)(std::move(fields));
		for(size_t i = 0; i < ret.v_array.size(); i++)
		{
			if(ret.v_array[i].type() != ty->getFieldAtIndex(i))
				sap::internal_error("mismatched field types!");
		}

		return ret;
	}

	Value Value::nullPointer()
	{
		auto ret = Value(Type::makeNullPtr());
		ret.v_pointer = nullptr;
		return ret;
	}

	Value Value::pointer(const Type* elm_type, const Value* value)
	{
		auto ret = Value(elm_type->pointerTo());
		ret.v_pointer = value;
		return ret;
	}

	Value Value::mutablePointer(const Type* elm_type, Value* value)
	{
		auto ret = Value(elm_type->mutablePointerTo());
		ret.v_pointer = value;
		return ret;
	}

	Value Value::optional(const Type* type, std::optional<Value> value)
	{
		auto ret = Value(type->optionalOf());
		new(&ret.v_array) decltype(v_array)();
		if(value.has_value())
			ret.v_array.push_back(std::move(*value));

		return ret;
	}


	Value Value::clone() const
	{
		this->ensure_not_moved_from();

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
		else if(m_type->isLength())
		{
			new(&val.v_length) decltype(v_length)(v_length);
		}
		else if(m_type->isTreeBlockObj())
		{
			new(&val.v_block_obj) decltype(v_block_obj)();
			val.v_block_obj = Value::clone_tbos(*v_block_obj);
		}
		else if(m_type->isTreeInlineObj())
		{
			new(&val.v_inline_obj) decltype(v_inline_obj)();
			val.v_inline_obj = Value::clone_tios(v_inline_obj);
		}
		else if(m_type->isArray() || m_type->isStruct() || m_type->isOptional() || m_type->isEnum())
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



	template <typename T>
	T Value::get() const
	{
		if constexpr(std::is_same_v<T, bool>)
			return this->getBool();

		else if constexpr(std::is_same_v<T, char32_t>)
			return this->getChar();

		else if constexpr(std::is_same_v<T, int64_t>)
			return this->getInteger();

		else if constexpr(std::is_same_v<T, double>)
			return this->getFloating();

		else
		{
			static_assert(std::is_same_v<T, T>, "unsupported type");
			return {};
		}
	}

	template char32_t Value::get<char32_t>() const;
	template int64_t Value::get<int64_t>() const;
	template double Value::get<double>() const;
	template bool Value::get<bool>() const;

	void Value::destroy()
	{
		if(m_type->isTreeInlineObj())
			v_inline_obj.~decltype(v_inline_obj)();
		else if(m_type->isTreeBlockObj())
			v_block_obj.~decltype(v_block_obj)();
		else if(m_type->isArray() || m_type->isStruct() || m_type->isOptional() || m_type->isEnum())
			v_array.~decltype(v_array)();
		else if(m_type->isLength())
			v_length.~decltype(v_length)();
	}

	void Value::steal_from(Value&& val)
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
		else if(m_type->isTreeBlockObj())
			new(&v_block_obj) decltype(v_block_obj)(std::move(val.v_block_obj));
		else if(m_type->isTreeInlineObj())
			new(&v_inline_obj) decltype(v_inline_obj)(std::move(val.v_inline_obj));
		else if(m_type->isArray() || m_type->isStruct() || m_type->isOptional() || m_type->isEnum())
			new(&v_array) decltype(v_array)(std::move(val.v_array));
		else if(m_type->isLength())
			new(&v_length) decltype(v_length)(std::move(val.v_length));
		else
			assert(false && "unreachable!");

		m_moved_from = false;
		val.m_moved_from = true;
	}

	auto Value::clone_tios(const InlineObjects& from) -> InlineObjects
	{
		InlineObjects ret {};
		ret.reserve(from.size());

		for(auto& tio : from)
		{
			// whatever is left here should only be text!
			if(auto txt = dynamic_cast<const tree::Text*>(tio.get()); txt)
				ret.emplace_back(new tree::Text(txt->contents(), txt->style()));

			else
				sap::internal_error("????? non-text TIOs leaked out!");
		}

		return ret;
	}

	std::unique_ptr<tree::BlockObject> Value::clone_tbos(const tree::BlockObject& from)
	{
		zpr::println("warning: cloning tbo!");
		if(auto para = dynamic_cast<const tree::Paragraph*>(&from); para != nullptr)
		{
			auto ret = std::make_unique<tree::Paragraph>();
			ret->addObjects(clone_tios(para->contents()));
			return ret;
		}
		else if(auto img = dynamic_cast<const tree::Image*>(&from); img != nullptr)
		{
			return std::make_unique<tree::Image>(img->image().clone(), img->size());
		}
		else if(auto scr = dynamic_cast<const tree::ScriptBlock*>(&from); scr != nullptr)
		{
			sap::internal_error("???? script TBO leaked out!");
		}
		else if(auto blk = dynamic_cast<const tree::BlockContainer*>(&from); blk != nullptr)
		{
			auto ret = std::make_unique<tree::BlockContainer>();
			for(auto& inner : blk->contents())
				ret->contents().push_back(clone_tbos(*inner));
			return ret;
		}
		else
		{
			sap::internal_error("?? unknown TBO type");
		}

		// TODO
		return nullptr;
	}
}
