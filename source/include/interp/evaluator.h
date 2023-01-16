// evaluator.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <deque>

#include "util.h"

#include "interp/ast.h"
#include "interp/value.h"

namespace sap
{
	struct Style;
}

namespace sap::tree
{
	struct InlineObject;
}

namespace sap::interp
{
	struct Evaluator;
	struct StackFrame
	{
		StackFrame* parent() const { return m_parent; }

		Value* valueOf(const Definition* defn);

		void setValue(const Definition* defn, Value value);
		Value* createTemporary(Value init);

		void dropTemporaries();
		size_t callDepth() const { return m_call_depth; }

		bool containsValue(const Value& value) const;

	private:
		explicit StackFrame(Evaluator* ev, StackFrame* parent, size_t call_depth)
		    : m_evaluator(ev)
		    , m_parent(parent)
		    , m_call_depth(call_depth)
		{
		}

		friend struct Evaluator;

		Evaluator* m_evaluator;
		StackFrame* m_parent = nullptr;
		size_t m_call_depth;
		std::unordered_map<const Definition*, Value> m_values;
		std::deque<Value> m_temporaries;
	};


	struct Evaluator
	{
		Evaluator();

		StackFrame& frame();
		[[nodiscard]] util::Defer<> pushFrame();
		[[nodiscard]] util::Defer<> pushCallFrame();
		void popFrame();

		void dropValue(Value&& value);
		Value castValue(Value value, const Type* to) const;

		StrErrorOr<const Style*> currentStyle() const;
		void pushStyle(const Style* style);
		StrErrorOr<const Style*> popStyle();

		StrErrorOr<std::vector<std::unique_ptr<tree::InlineObject>>> convertValueToText(Value&& value);

	private:
		std::vector<std::unique_ptr<StackFrame>> m_stack_frames;
		std::vector<const Style*> m_style_stack;
	};
}
