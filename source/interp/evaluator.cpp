// evaluator.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

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
			         ? Value::mutablePointer(to->toPointer()->elementType(), nullptr)
			         : Value::pointer(to->toPointer()->elementType(), nullptr);
		}
		else if(from_type->isMutablePointer() && to->isPointer()
		        && from_type->toPointer()->elementType() == to->toPointer()->elementType())
		{
			return Value::pointer(to->toPointer()->elementType(), value.getPointer());
		}
		else if(from_type->isInteger() && to->isFloating())
		{
			return Value::floating(static_cast<double>(value.getInteger()));
		}
		else
		{
			return value;
		}
	}

	void Evaluator::dropValue(Value&& value)
	{
		// TODO: call destructors
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
