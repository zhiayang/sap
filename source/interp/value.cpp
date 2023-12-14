// value.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <utf8proc/utf8proc.h>

#include "tree/image.h"
#include "tree/wrappers.h"
#include "tree/container.h"
#include "tree/paragraph.h"

#include "layout/layout_object.h"

#include "interp/value.h"

namespace sap::interp
{
	const Type* Value::type() const
	{
		return m_type;
	}

#define REIFY_GEN_FUNC(field)                                \
	do                                                       \
	{                                                        \
		if(m_gen_func)                                       \
		{                                                    \
			new(&field) decltype(field)(m_gen_func().field); \
		}                                                    \
	} while(false)

	bool Value::hasGenerator() const
	{
		return m_gen_func != nullptr;
	}

	size_t Value::getUnionVariantIndex() const
	{
		return m_union_case_idx;
	}

	bool Value::getBool() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isBool());
		REIFY_GEN_FUNC(v_bool);

		return v_bool;
	}

	char32_t Value::getChar() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isChar());
		REIFY_GEN_FUNC(v_char);

		return v_char;
	}

	double Value::getFloating() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isFloating());
		REIFY_GEN_FUNC(v_floating);

		return v_floating;
	}

	int64_t Value::getInteger() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isInteger());
		REIFY_GEN_FUNC(v_integer);

		return v_integer;
	}

	DynLength Value::getLength() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isLength());
		REIFY_GEN_FUNC(v_length);

		return v_length;
	}

	auto Value::getFunction() const -> FnType
	{
		this->ensure_not_moved_from();
		assert(m_type->isFunction());
		REIFY_GEN_FUNC(v_function);

		return v_function;
	}


	const std::vector<Value>& Value::getArray() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isArray());
		REIFY_GEN_FUNC(v_array);

		return v_array;
	}

	std::vector<Value> Value::takeArray() &&
	{
		this->ensure_not_moved_from();
		assert(m_type->isArray());
		REIFY_GEN_FUNC(v_array);

		return std::move(v_array);
	}

	const Value& Value::getEnumerator() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isEnum());
		REIFY_GEN_FUNC(v_array);

		return v_array[0];
	}

	Value Value::takeEnumerator() &&
	{
		this->ensure_not_moved_from();
		assert(m_type->isEnum());
		REIFY_GEN_FUNC(v_array);

		return std::move(v_array[0]);
	}




	std::string Value::getUtf8String() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isArray());
		REIFY_GEN_FUNC(v_array);


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
		assert(m_type->isArray());
		REIFY_GEN_FUNC(v_array);

		std::u32string ret {};
		ret.reserve(v_array.size());

		for(size_t i = 0; i < v_array.size(); i++)
			ret += v_array[i].getChar();

		return ret;
	}


	const Value& Value::getUnionUnderlyingStruct() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isUnion());
		REIFY_GEN_FUNC(v_array);

		return v_array[0];
	}

	Value& Value::getUnionUnderlyingStruct()
	{
		this->ensure_not_moved_from();
		assert(m_type->isUnion());
		REIFY_GEN_FUNC(v_array);

		return v_array[0];
	}

	Value Value::takeUnionUnderlyingStruct() &&
	{
		this->ensure_not_moved_from();
		assert(m_type->isUnion());
		REIFY_GEN_FUNC(v_array);

		return std::move(v_array[0]);
	}


	Value& Value::getStructField(size_t idx)
	{
		this->ensure_not_moved_from();
		assert(m_type->isStruct());
		REIFY_GEN_FUNC(v_array);

		return v_array[idx];
	}

	const Value& Value::getStructField(size_t idx) const
	{
		this->ensure_not_moved_from();
		assert(this->isStruct());
		REIFY_GEN_FUNC(v_array);

		return v_array[idx];
	}

	Value& Value::getStructField(zst::str_view name)
	{
		this->ensure_not_moved_from();
		assert(m_type->isStruct());
		REIFY_GEN_FUNC(v_array);

		return v_array[m_type->toStruct()->getFieldIndex(name)];
	}

	const Value& Value::getStructField(zst::str_view name) const
	{
		this->ensure_not_moved_from();
		assert(m_type->isStruct());
		REIFY_GEN_FUNC(v_array);

		return v_array[m_type->toStruct()->getFieldIndex(name)];
	}


	std::vector<Value> Value::takeStructFields() &&
	{
		this->ensure_not_moved_from();
		assert(m_type->isStruct());
		REIFY_GEN_FUNC(v_array);

		return std::move(v_array);
	}

	const std::vector<Value>& Value::getStructFields() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isStruct());
		REIFY_GEN_FUNC(v_array);

		return v_array;
	}

	const Value* Value::getPointer() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isPointer());
		REIFY_GEN_FUNC(v_pointer);

		return v_pointer;
	}

	Value* Value::getMutablePointer() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isMutablePointer());
		REIFY_GEN_FUNC(v_pointer);

		return const_cast<Value*>(v_pointer);
	}

	tree::BlockObject* Value::getTreeBlockObjectRef() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isTreeBlockObjRef());
		REIFY_GEN_FUNC(v_block_obj_ref);

		return v_block_obj_ref;
	}

	layout::LayoutObject* Value::getLayoutObjectRef() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isLayoutObjectRef());
		REIFY_GEN_FUNC(v_layout_obj_ref);

		return v_layout_obj_ref;
	}

	tree::InlineSpan* Value::getTreeInlineObjectRef() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isTreeInlineObjRef());
		REIFY_GEN_FUNC(v_inline_obj_ref);

		return v_inline_obj_ref;
	}

	std::optional<const Value*> Value::getOptional() const
	{
		this->ensure_not_moved_from();
		assert(m_type->isOptional());
		REIFY_GEN_FUNC(v_array);

		if(v_array.empty())
			return std::nullopt;

		return &v_array[0];
	}

	std::optional<Value*> Value::getOptional()
	{
		this->ensure_not_moved_from();
		assert(m_type->isOptional());
		REIFY_GEN_FUNC(v_array);

		if(v_array.empty())
			return std::nullopt;

		return &v_array[0];
	}

	std::optional<Value> Value::takeOptional() &&
	{
		this->ensure_not_moved_from();
		assert(m_type->isOptional());
		REIFY_GEN_FUNC(v_array);

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
		REIFY_GEN_FUNC(v_array);

		return v_array.size() > 0;
	}

	auto Value::getTreeInlineObj() const -> const tree::InlineSpan&
	{
		this->ensure_not_moved_from();
		assert(m_type->isTreeInlineObj());
		assert(not m_gen_func);

		return *v_inline_obj;
	}

	auto Value::takeTreeInlineObj() && -> zst::SharedPtr<tree::InlineSpan>
	{
		this->ensure_not_moved_from();
		assert(m_type->isTreeInlineObj());
		assert(not m_gen_func);

		return std::move(v_inline_obj);
	}

	auto Value::getTreeBlockObj() const -> const tree::BlockObject&
	{
		this->ensure_not_moved_from();
		assert(m_type->isTreeBlockObj());
		assert(not m_gen_func);

		return *v_block_obj;
	}

	auto Value::takeTreeBlockObj() && -> zst::SharedPtr<tree::BlockObject>
	{
		this->ensure_not_moved_from();
		assert(m_type->isTreeBlockObj());
		assert(not m_gen_func);

		return std::move(v_block_obj);
	}

	auto Value::getLayoutObject() const -> const layout::LayoutObject&
	{
		this->ensure_not_moved_from();
		assert(m_type->isLayoutObject());
		assert(not m_gen_func);

		return *v_layout_obj;
	}

	auto Value::takeLayoutObject() && -> std::unique_ptr<layout::LayoutObject>
	{
		this->ensure_not_moved_from();
		assert(m_type->isLayoutObject());
		assert(not m_gen_func);

		return std::move(v_layout_obj);
	}


	std::u32string Value::toString() const
	{
		this->ensure_not_moved_from();
		if(m_gen_func)
			return m_gen_func().toString();

		if(m_type->isChar())
		{
			return &v_char;
		}
		else if(m_type->isBool())
		{
			return v_bool ? U"true" : U"false";
		}
		else if(m_type->isLength())
		{
			return unicode::u32StringFromUtf8(zpr::sprint("{}{}", v_length.value(),
			    DynLength::unitToString(v_length.unit())));
		}
		else if(m_type->isInteger())
		{
			return unicode::u32StringFromUtf8(std::to_string(v_integer));
		}
		else if(m_type->isFloating())
		{
			return unicode::u32StringFromUtf8(std::to_string(v_floating));
		}
		else if(m_type->isFunction())
		{
			return U"function";
		}
		else if(m_type->isTreeBlockObj())
		{
			return U"Block{...}";
		}
		else if(m_type->isTreeInlineObj())
		{
			return U"Inline{...}";
		}
		else if(m_type->isLayoutObject())
		{
			return U"LayoutObject{...}";
		}
		else if(m_type->isTreeBlockObjRef())
		{
			return U"BlockRef";
		}
		else if(m_type->isTreeInlineObjRef())
		{
			return U"InlineRef";
		}
		else if(m_type->isLayoutObjectRef())
		{
			return U"LayoutObjectRef";
		}
		else if(m_type->isOptional())
		{
			if(v_array.empty())
				return U"empty";
			else
				return v_array[0].toString();
		}
		else if(m_type->isUnion() || m_type->isStruct())
		{
			return unicode::u32StringFromUtf8(m_type->str());
		}
		else if(m_type->isArray())
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
	bool Value::isUnion() const
	{
		return m_type->isUnion();
	}
	bool Value::isString() const
	{
		return m_type->isArray() && m_type->arrayElement()->isChar();
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
	bool Value::isLayoutObject() const
	{
		return m_type->isLayoutObject();
	}

	bool Value::isPointer() const
	{
		return m_type->isPointer();
	}

	bool Value::isTreeInlineObjRef() const
	{
		return m_type->isTreeInlineObjRef();
	}

	bool Value::isTreeBlockObjRef() const
	{
		return m_type->isTreeBlockObjRef();
	}

	bool Value::isLayoutObjectRef() const
	{
		return m_type->isLayoutObjectRef();
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

	Value Value::string(const std::string& _str)
	{
		auto u32str = unicode::u32StringFromUtf8(_str.data());

		auto ret = Value(Type::makeString());
		new(&ret.v_array) decltype(ret.v_array)();
		ret.v_array.resize(u32str.size());

		for(size_t i = 0; i < u32str.size(); i++)
			ret.v_array[i] = Value::character(u32str[i]);

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

	Value Value::array(const Type* elm, std::vector<Value> arr, bool variadic)
	{
		auto ret = Value(Type::makeArray(elm, variadic));
		new(&ret.v_array) decltype(ret.v_array)();
		ret.v_array.resize(arr.size());

		for(size_t i = 0; i < arr.size(); i++)
			ret.v_array[i] = std::move(arr[i]);

		return ret;
	}

	Value Value::treeInlineObject(zst::SharedPtr<tree::InlineSpan> obj)
	{
		auto ret = Value(Type::makeTreeInlineObj());
		new(&ret.v_inline_obj) decltype(ret.v_inline_obj)(std::move(obj));
		return ret;
	}

	Value Value::treeBlockObject(zst::SharedPtr<tree::BlockObject> obj)
	{
		auto ret = Value(Type::makeTreeBlockObj());
		new(&ret.v_block_obj) decltype(ret.v_block_obj)(std::move(obj));
		return ret;
	}

	Value Value::layoutObject(std::unique_ptr<layout::LayoutObject> obj)
	{
		auto ret = Value(Type::makeLayoutObject());
		new(&ret.v_layout_obj) decltype(ret.v_layout_obj)(std::move(obj));
		return ret;
	}

	Value Value::structure(const StructType* ty, std::vector<Value> fields)
	{
		auto ret = Value(ty);
		new(&ret.v_array) decltype(ret.v_array)(std::move(fields));
		for(size_t i = 0; i < ret.v_array.size(); i++)
		{
			if(ret.v_array[i].type() != ty->getFieldAtIndex(i))
			{
				sap::internal_error("mismatched field types! expected '{}', got '{}'", ty->getFieldAtIndex(i),
				    ret.v_array[i].type());
			}
		}

		return ret;
	}

	Value Value::unionVariant(const UnionType* type, size_t case_idx, Value case_value_as_struct)
	{
		auto ret = Value(type);
		ret.m_union_case_idx = case_idx;
		new(&ret.v_array) decltype(ret.v_array)();

		auto case_type = type->getCaseAtIndex(case_idx);
		for(size_t i = 0; i < case_value_as_struct.v_array.size(); i++)
		{
			if(case_value_as_struct.v_array[i].type() != case_type->getFieldAtIndex(i))
			{
				sap::internal_error("mismatched field types! expected '{}', got '{}'", case_type->getFieldAtIndex(i),
				    case_value_as_struct.v_array[i].type());
			}
		}

		ret.v_array.push_back(std::move(case_value_as_struct));
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

	Value Value::treeInlineObjectRef(tree::InlineSpan* obj)
	{
		auto ret = Value(Type::makeTreeInlineObjRef());
		new(&ret.v_inline_obj_ref) decltype(v_inline_obj_ref)();
		ret.v_inline_obj_ref = obj;

		return ret;
	}

	Value Value::treeBlockObjectRef(tree::BlockObject* obj)
	{
		auto ret = Value(Type::makeTreeBlockObjRef());
		new(&ret.v_block_obj_ref) decltype(v_block_obj_ref)();
		ret.v_block_obj_ref = obj;

		return ret;
	}

	Value Value::layoutObjectRef(layout::LayoutObject* obj)
	{
		auto ret = Value(Type::makeLayoutObjectRef());
		new(&ret.v_layout_obj_ref) decltype(v_layout_obj_ref)();
		ret.v_layout_obj_ref = obj;

		return ret;
	}

	Value Value::fromGenerator(const Type* type, std::function<Value()> gen_func)
	{
		auto ret = Value(type);
		ret.m_gen_func = std::move(gen_func);

		return ret;
	}



	Value Value::clone() const
	{
		this->ensure_not_moved_from();

		auto ret = Value(m_type);
		if(m_type->isBool())
		{
			REIFY_GEN_FUNC(v_bool);
			ret.v_bool = v_bool;
		}
		else if(m_type->isChar())
		{
			REIFY_GEN_FUNC(v_char);
			ret.v_char = v_char;
		}
		else if(m_type->isInteger())
		{
			REIFY_GEN_FUNC(v_integer);
			ret.v_integer = v_integer;
		}
		else if(m_type->isFloating())
		{
			REIFY_GEN_FUNC(v_floating);
			ret.v_floating = v_floating;
		}
		else if(m_type->isPointer() || m_type->isNullPtr())
		{
			REIFY_GEN_FUNC(v_pointer);
			ret.v_pointer = v_pointer;
		}
		else if(m_type->isFunction())
		{
			REIFY_GEN_FUNC(v_function);
			ret.v_function = v_function;
		}
		else if(m_type->isLength())
		{
			REIFY_GEN_FUNC(v_length);
			new(&ret.v_length) decltype(v_length)(v_length);
		}
		else if(m_type->isArray() || m_type->isStruct() || m_type->isOptional() //
		        || m_type->isEnum() || m_type->isUnion())
		{
			REIFY_GEN_FUNC(v_array);

			new(&ret.v_array) decltype(v_array)();
			for(auto& e : v_array)
				ret.v_array.push_back(e.clone());

			// just unconditionally set this, it doesn't matter
			ret.m_union_case_idx = m_union_case_idx;
		}
		else if(m_type->isTreeBlockObjRef())
		{
			REIFY_GEN_FUNC(v_block_obj_ref);
			ret.v_block_obj_ref = v_block_obj_ref;
		}
		else if(m_type->isLayoutObjectRef())
		{
			REIFY_GEN_FUNC(v_layout_obj_ref);
			ret.v_layout_obj_ref = v_layout_obj_ref;
		}
		else if(m_type->isTreeInlineObjRef())
		{
			REIFY_GEN_FUNC(v_inline_obj_ref);
			ret.v_inline_obj_ref = v_inline_obj_ref;
		}
		else if(m_type->isTreeBlockObj())
		{
			sap::internal_error("cannot clone 'Block' value");
		}
		else if(m_type->isTreeInlineObj())
		{
			sap::internal_error("cannot clone 'Inline' value");
		}
		else if(m_type->isLayoutObject())
		{
			sap::internal_error("cannot clone 'LayoutObject' value");
		}
		else
		{
			zpr::println("weird type: {}", m_type);
			assert(false && "unreachable!");
		}

		if(m_gen_func)
			m_gen_func = nullptr;

		return ret;
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
		if(m_gen_func || m_moved_from)
			return;

		else if(m_type->isTreeInlineObj())
			std::destroy_at(&v_inline_obj);

		else if(m_type->isTreeBlockObj())
			std::destroy_at(&v_block_obj);

		else if(m_type->isPointer() && m_type->pointerElement()->isTreeInlineObj())
			std::destroy_at(&v_inline_obj_ref);

		else if(m_type->isArray() || m_type->isStruct() || m_type->isOptional() //
		        || m_type->isEnum() || m_type->isUnion())
			std::destroy_at(&v_array);

		else if(m_type->isLength())
			std::destroy_at(&v_length);
	}

	void Value::steal_from(Value&& val)
	{
		m_type = val.m_type;
		if(val.m_gen_func)
			m_gen_func = std::move(val.m_gen_func);

		else if(m_type->isBool())
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

		else if(m_type->isLayoutObject())
			new(&v_layout_obj) decltype(v_layout_obj)(std::move(val.v_layout_obj));

		else if(m_type->isArray() || m_type->isStruct() || m_type->isOptional() //
		        || m_type->isEnum() || m_type->isUnion())
		{
			new(&v_array) decltype(v_array)(std::move(val.v_array));
			m_union_case_idx = val.m_union_case_idx;
		}
		else if(m_type->isLength())
			new(&v_length) decltype(v_length)(std::move(val.v_length));

		else if(m_type->isTreeBlockObjRef())
			v_block_obj_ref = std::move(val.v_block_obj_ref);

		else if(m_type->isLayoutObjectRef())
			v_layout_obj_ref = std::move(val.v_layout_obj_ref);

		else if(m_type->isTreeInlineObjRef())
			new(&v_inline_obj_ref) decltype(v_inline_obj_ref)(std::move(val.v_inline_obj_ref));

		else
			assert(false && "unreachable!");

		m_moved_from = false;
		val.m_moved_from = true;
	}
}
