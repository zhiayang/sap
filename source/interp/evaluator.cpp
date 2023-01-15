// evaluator.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/paragraph.h"

#include "interp/ast.h"
#include "interp/evaluator.h"

namespace sap::interp
{
	Evaluator::Evaluator()
	{
		// always start with a top level frame.
		m_stack_frames.push_back(std::unique_ptr<StackFrame>(new StackFrame(this, nullptr)));
	}

	Value Evaluator::castValue(Value value, const Type* to) const
	{
		if(value.type() == to)
			return value;

		// TODO: handle 'any'
		auto from_type = value.type();
		if(from_type->isNullPtr() && to->isPointer())
		{
			return to->isMutablePointer() //
			         ? Value::mutablePointer(to->pointerElement(), nullptr)
			         : Value::pointer(to->pointerElement(), nullptr);
		}
		else if(from_type->isMutablePointer() && to->isPointer() && from_type->pointerElement() == to->pointerElement())
		{
			return Value::pointer(to->pointerElement(), value.getPointer());
		}
		else if(from_type->isInteger() && to->isFloating())
		{
			return Value::floating(static_cast<double>(value.getInteger()));
		}
		else if(to->isOptional() && to->optionalElement() == from_type)
		{
			return Value::optional(from_type, std::move(value));
		}
		else if(to->isOptional() && from_type->isNullPtr())
		{
			return Value::optional(to->optionalElement(), std::nullopt);
		}
		else if(to->isOptional())
		{
			auto elm = to->optionalElement();
			return Value::optional(elm, this->castValue(std::move(value), elm));
		}
		else if(from_type->isEnum() && from_type->toEnum()->elementType() == to)
		{
			return std::move(value).takeEnumerator();
		}
		else
		{
			return value;
		}
	}

	ErrorOr<std::vector<std::unique_ptr<tree::InlineObject>>> Evaluator::convertValueToText(Value&& value)
	{
		std::vector<std::unique_ptr<tree::InlineObject>> ret {};

		if(value.isTreeInlineObj())
		{
			auto objs = std::move(value).takeTreeInlineObj();
			ret.insert(ret.end(), std::move_iterator(objs.begin()), std::move_iterator(objs.end()));
		}
		else if(value.isPrintable())
		{
			ret.emplace_back(new tree::Text(value.toString()));
		}
		else
		{
			return ErrFmt("cannot convert value of type '{}' into text", value.type());
		}

		return Ok(std::move(ret));
	}


	StackFrame& Evaluator::frame()
	{
		return *m_stack_frames.back();
	}

	[[nodiscard]] util::Defer<> Evaluator::pushFrame()
	{
		auto cur = m_stack_frames.back().get();
		m_stack_frames.push_back(std::unique_ptr<StackFrame>(new StackFrame(this, cur)));
		return util::Defer<>([this]() {
			this->popFrame();
		});
	}

	void Evaluator::popFrame()
	{
		assert(not m_stack_frames.empty());
		m_stack_frames.pop_back();
	}


	void Evaluator::dropValue(Value&& value)
	{
		// TODO: call destructors
	}


	ErrorOr<const Style*> Evaluator::currentStyle() const
	{
		if(m_style_stack.empty())
			return ErrFmt("no style set!");

		return Ok(m_style_stack.back());
	}

	void Evaluator::pushStyle(const Style* style)
	{
		m_style_stack.push_back(style);
	}

	ErrorOr<const Style*> Evaluator::popStyle()
	{
		if(m_style_stack.empty())
			return ErrFmt("no style set!");

		auto ret = m_style_stack.back();
		m_style_stack.pop_back();
		return Ok(ret);
	}




	Value* StackFrame::valueOf(const Definition* defn)
	{
		if(auto it = m_values.find(defn); it == m_values.end())
			return nullptr;
		else
			return &it->second;
	}

	void StackFrame::setValue(const Definition* defn, Value value)
	{
		m_values[defn] = std::move(value);
	}

	Value* StackFrame::createTemporary(Value init)
	{
		return &m_temporaries.emplace_back(std::move(init));
	}

	void StackFrame::dropTemporaries()
	{
		while(not m_temporaries.empty())
		{
			m_evaluator->dropValue(std::move(m_temporaries.back()));
			m_temporaries.pop_back();
		}
	}

}
