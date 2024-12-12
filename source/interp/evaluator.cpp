// evaluator.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cmath>
#include <array>

#include "tree/wrappers.h"
#include "tree/paragraph.h"

#include "layout/base.h"
#include "layout/line.h"
#include "layout/document.h"

#include "interp/ast.h"
#include "interp/interp.h"
#include "interp/evaluator.h"
#include "interp/builtin_types.h"

namespace sap::interp
{
	Evaluator::Evaluator(Interpreter* cs) : m_interp(cs)
	{
		// always start with a top level frame.
		m_stack_frames.push_back(std::unique_ptr<StackFrame>(new StackFrame(this, nullptr, 0)));
	}

	[[nodiscard]] Location Evaluator::loc() const
	{
		return m_location_stack.back();
	}

	[[nodiscard]] util::Defer<> Evaluator::pushLocation(const Location& loc)
	{
		m_location_stack.push_back(loc);
		return util::Defer([this]() { this->popLocation(); });
	}

	void Evaluator::popLocation()
	{
		assert(m_location_stack.size() > 0);
		m_location_stack.pop_back();
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
			auto casted = this->castValue(std::move(value), elm);
			if(casted.type() == elm)
				return Value::optional(elm, std::move(casted));
			else
				return casted;
		}
		else if(from_type->isEnum() && from_type->toEnum()->elementType() == to)
		{
			return std::move(value).takeEnumerator();
		}
		else if(from_type->isArray() && to->isArray()
		        && (from_type->arrayElement()->isVoid() || to->arrayElement()->isVoid()))
		{
			return Value::array(to->arrayElement(), std::move(value).takeArray());
		}
		else if(from_type->isPointer() && to->isPointer() && from_type->pointerElement()->isArray()
		        && to->pointerElement()->isArray()
		        && (from_type->pointerElement()->arrayElement()->isVoid()
		            || to->pointerElement()->arrayElement()->isVoid()))
		{
			assert(from_type->isMutablePointer() || not to->isMutablePointer());
			if(to->isMutablePointer())
				return Value::mutablePointer(to->pointerElement(), value.getMutablePointer());
			else
				return Value::pointer(to->pointerElement(), value.getPointer());
		}
		else
		{
			return value;
		}
	}

	ErrorOr<zst::SharedPtr<tree::InlineSpan>> Evaluator::convertValueToText(Value&& value)
	{
		if(value.isTreeInlineObj())
		{
			return Ok(std::move(value).takeTreeInlineObj());
		}
		else if(value.isPrintable())
		{
			auto ret = zst::make_shared<tree::InlineSpan>(/* glue: */ false);
			ret->addObject(zst::make_shared<tree::Text>(value.toString()));

			return OkMove(ret);
		}
		else
		{
			return ErrMsg(this, "cannot convert value of type '{}' into text", value.type());
		}
	}


	StackFrame& Evaluator::frame()
	{
		return *m_stack_frames.back();
	}

	[[nodiscard]] util::Defer<> Evaluator::pushFrame()
	{
		auto cur = m_stack_frames.back().get();
		m_stack_frames.push_back(std::unique_ptr<StackFrame>(new StackFrame(this, cur, cur->callDepth())));
		return util::Defer<>([this]() { this->popFrame(); });
	}

	[[nodiscard]] util::Defer<> Evaluator::pushCallFrame()
	{
		auto cur = m_stack_frames.back().get();
		m_stack_frames.push_back(std::unique_ptr<StackFrame>(new StackFrame(this, cur, 1 + cur->callDepth())));
		return util::Defer<>([this]() { this->popFrame(); });
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


	[[nodiscard]] const Style& Evaluator::currentStyle() const
	{
		if(m_style_stack.empty())
			return Style::empty();

		return m_style_stack.back();
	}

	void Evaluator::pushStyle(const Style& style)
	{
		auto tmp = style;
		if(not m_style_stack.empty())
			tmp = m_style_stack.back().extendWith(style);

		m_style_stack.push_back(std::move(tmp));
	}

	Style Evaluator::popStyle()
	{
		if(m_style_stack.empty())
			return Style::empty();

		auto ret = m_style_stack.back();
		m_style_stack.pop_back();
		return ret;
	}

	[[nodiscard]] Value& Evaluator::getStructFieldContext() const
	{
		assert(not m_struct_field_context_stack.empty());
		return *m_struct_field_context_stack.back();
	}

	[[nodiscard]] util::Defer<> Evaluator::pushStructFieldContext(Value* struct_value)
	{
		m_struct_field_context_stack.push_back(struct_value);
		return util::Defer([this]() { m_struct_field_context_stack.pop_back(); });
	}


	[[nodiscard]] const BlockContext& Evaluator::getBlockContext() const
	{
		return m_block_context_stack.back();
	}

	[[nodiscard]] util::Defer<> Evaluator::pushBlockContext(std::optional<const tree::BlockObject*> obj)
	{
		m_block_context_stack.push_back(BlockContext { .obj = std::move(obj) });
		return util::Defer([this]() { this->popBlockContext(); });
	}

	void Evaluator::popBlockContext()
	{
		m_block_context_stack.pop_back();
	}

	void Evaluator::requestLayout()
	{
		m_relayout_requested = true;
	}

	bool Evaluator::layoutRequested() const
	{
		return m_relayout_requested;
	}

	void Evaluator::commenceLayoutPass(size_t pass_num)
	{
		m_relayout_requested = false;
		m_global_state.layout_pass = pass_num;

		util::log("layout pass: {}", pass_num);
	}

	void Evaluator::setDocument(layout::Document* document)
	{
		m_document = document;
		m_document_proxy_value = builtin::BS_DocumentProxy::make(this, m_document);
	}

	const GlobalState& Evaluator::state() const
	{
		return m_global_state;
	}

	GlobalState& Evaluator::state()
	{
		return m_global_state;
	}



	bool Evaluator::isGlobalValue(const cst::Definition* defn) const
	{
		return m_global_values.contains(defn);
	}

	void Evaluator::setGlobalValue(const cst::Definition* defn, Value val)
	{
		m_global_values[defn] = std::move(val);
	}

	Value* Evaluator::getGlobalValue(const cst::Definition* defn)
	{
		if(auto it = m_global_values.find(defn); it != m_global_values.end())
			return &it->second;

		return nullptr;
	}

	ErrorOr<void> Evaluator::addAbsolutelyPositionedBlockObject(zst::SharedPtr<tree::BlockObject> tbo_,
	    layout::AbsolutePagePos abs_pos)
	{
		if(m_document == nullptr)
			return ErrMsg(this, "cannot output objects in this context");

		auto tbo = m_interp->retainBlockObject(std::move(tbo_));

		// TODO: calculate the available space properly.
		auto avail_space = Size2d(Length(INFINITY), Length(INFINITY));
		auto obj = TRY(tbo->createLayoutObject(m_interp, this->currentStyle(), avail_space)).object;

		if(obj.has_value())
		{
			auto& pl = m_document->pageLayout();
			auto cursor = pl.newCursorAtPosition(abs_pos);
			auto ptr = pl.addObject(std::move(*obj));
			ptr->computePosition(cursor);
		}

		return Ok();
	}

	Value* Evaluator::addToHeap(Value value)
	{
		return &m_value_heap.emplace_back(std::move(value));
	}

	Value Evaluator::addToHeapAndGetPointer(Value value)
	{
		auto ptr = this->addToHeap(std::move(value));
		return Value::mutablePointer(ptr->type(), ptr);
	}

	ErrorOr<EvalResult> Evaluator::call(const cst::Definition* defn, std::vector<Value>& args)
	{
		auto _ = this->pushCallFrame();

		if(auto builtin_defn = dynamic_cast<const cst::BuiltinFunctionDefn*>(defn); builtin_defn != nullptr)
		{
			return builtin_defn->function(this, args);
		}
		else if(auto func_defn = dynamic_cast<const cst::FunctionDefn*>(defn); func_defn != nullptr)
		{
			return func_defn->call(this, args);
		}
		else if(auto var_defn = dynamic_cast<const cst::VariableDefn*>(defn); var_defn != nullptr)
		{
			Value* val = nullptr;
			if(var_defn->is_global)
			{
				if((val = this->getGlobalValue(var_defn)) == nullptr)
					return ErrMsg(this, "value '{}' was not defined", var_defn->declaration->name);
			}
			else
			{
				if((val = this->frame().valueOf(var_defn)) == nullptr)
					return ErrMsg(this, "value '{}' was not defined", var_defn->declaration->name);
			}

			auto ret = val->getFunction()(this->interpreter(), args);
			if(ret.has_value())
				return EvalResult::ofValue(std::move(*ret));
			else
				return EvalResult::ofVoid();
		}
		else
		{
			return ErrMsg(this, "not implemented");
		}
	}



	Value* StackFrame::valueOf(const cst::Definition* defn)
	{
		if(auto it = m_values.find(defn); it == m_values.end())
			return nullptr;
		else
			return &it->second;
	}

	void StackFrame::setValue(const cst::Definition* defn, Value value)
	{
		m_values[defn] = std::move(value);
	}

	bool StackFrame::containsValue(const Value& value) const
	{
		for(auto& [defn, val] : m_values)
		{
			if(&val == &value)
				return true;
		}

		return false;
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
