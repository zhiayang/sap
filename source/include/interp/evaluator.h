// evaluator.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <deque>

#include "util.h"

#include "interp/ast.h"
#include "interp/value.h"

#include "layout/cursor.h"

namespace sap
{
	struct Style;
}

namespace sap::tree
{
	struct BlockObject;
	struct InlineObject;
}

namespace sap::layout
{
	struct PageCursor;
	struct RelativePos;
}

namespace sap::interp
{
	struct Evaluator;
	struct Interpreter;
	struct OutputContext;


	struct OutputContext
	{
		using TioPtr = std::unique_ptr<tree::InlineObject>;
		using LayoutInlineObjectFn = std::function<ErrorOr<void>(TioPtr)>;

		using TboPtr = std::unique_ptr<tree::BlockObject>;
		using TboLayoutResult = std::pair<layout::PageCursor, layout::LayoutObject*>;
		using LayoutBlockObjectFn = std::function<ErrorOr<TboLayoutResult>(TboPtr, std::optional<layout::PageCursor>)>;

		using SetLayoutCursorFn = std::function<ErrorOr<void>(layout::PageCursor)>;

		std::optional<LayoutBlockObjectFn> add_block_object = std::nullopt;
		std::optional<LayoutInlineObjectFn> add_inline_object = std::nullopt;
		std::optional<SetLayoutCursorFn> set_layout_cursor = std::nullopt;
	};

	// for tracking layout hierarchy in the evaluator
	struct BlockContext
	{
		layout::PageCursor cursor;
		layout::RelativePos parent_pos;
		std::optional<const tree::BlockObject*> obj;
		OutputContext output_context;
		const layout::PageCursor* cursor_ref;
	};

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
		explicit Evaluator(Interpreter* cs);
		Interpreter* interpreter() { return m_interp; }

		StackFrame& frame();
		[[nodiscard]] util::Defer<> pushFrame();
		[[nodiscard]] util::Defer<> pushCallFrame();
		void popFrame();

		void dropValue(Value&& value);
		Value castValue(Value value, const Type* to) const;

		[[nodiscard]] Location loc() const;
		[[nodiscard]] util::Defer<> pushLocation(const Location& loc);
		void popLocation();

		bool isGlobalValue(const Definition* defn) const;
		void setGlobalValue(const Definition* defn, Value val);
		Value* getGlobalValue(const Definition* defn);

		ErrorOr<std::vector<std::unique_ptr<tree::InlineObject>>> convertValueToText(Value&& value);

		const Style* currentStyle() const;
		void pushStyle(const Style* style);
		const Style* popStyle();

		const BlockContext& getBlockContext() const;
		[[nodiscard]] util::Defer<> pushBlockContext(const layout::PageCursor& cursor,
		    std::optional<const tree::BlockObject*> obj,
		    OutputContext output_ctx);

		void popBlockContext();


	private:
		Interpreter* m_interp;

		std::vector<std::unique_ptr<StackFrame>> m_stack_frames;

		std::vector<const Style*> m_style_stack;
		std::vector<BlockContext> m_block_context_stack;

		std::vector<Location> m_location_stack;

		std::unordered_map<const Definition*, Value> m_global_values;
	};
}
