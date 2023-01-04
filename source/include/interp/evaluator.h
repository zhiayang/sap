// evaluator.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <deque>

#include "util.h"

#include "interp/ast.h"
#include "interp/value.h"

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

		Value* valueOf(const Definition* defn)
		{
			if(auto it = m_values.find(defn); it == m_values.end())
				return nullptr;
			else
				return &it->second;
		}

		void setValue(const Definition* defn, Value value) { m_values[defn] = std::move(value); }
		Value* createTemporary(Value init) { return &m_temporaries.emplace_back(std::move(init)); }

		void dropTemporaries();

	private:
		explicit StackFrame(Evaluator* ev, StackFrame* parent) : m_evaluator(ev), m_parent(parent) { }

		friend struct Evaluator;

		Evaluator* m_evaluator;
		StackFrame* m_parent = nullptr;
		std::unordered_map<const Definition*, Value> m_values;
		std::deque<Value> m_temporaries;
	};


	struct Evaluator
	{
		Evaluator();

		StackFrame& frame() { return *m_stack_frames.back(); }
		[[nodiscard]] auto pushFrame()
		{
			auto cur = m_stack_frames.back().get();
			m_stack_frames.push_back(std::unique_ptr<StackFrame>(new StackFrame(this, cur)));
			return util::Defer([this]() {
				this->popFrame();
			});
		}

		void popFrame()
		{
			assert(not m_stack_frames.empty());
			m_stack_frames.pop_back();
		}

		void dropValue(Value&& value);
		Value castValue(Value value, const Type* to) const;

		ErrorOr<std::vector<std::unique_ptr<tree::InlineObject>>> convertValueToText(Value&& value);

	private:
		std::vector<std::unique_ptr<StackFrame>> m_stack_frames;
	};
}
