// base.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/units.h"
#include "sap/style.h" // for Stylable

#include "layout/layout_object.h"

namespace sap::layout
{
	struct PageCursor;
	struct LayoutBase;
	struct LayoutObject;
}

namespace sap::interp
{
	struct Block;
	struct Interpreter;
	struct FunctionCall;
}

namespace sap::tree
{
	/*
	    This defines the main tree structure for the entire system. This acts as the "AST" of the entire
	    document, and is what the metaprogram will traverse/manipulate (in the future).
	*/

	struct LayoutResult
	{
		layout::PageCursor cursor;
		std::vector<std::unique_ptr<layout::LayoutObject>> objects;

		template <typename... Objs>
		static LayoutResult make(layout::PageCursor cursor, Objs&&... objs)
		{
			return LayoutResult {
				.cursor = std::move(cursor),
				.objects = util::vectorOf<std::unique_ptr<layout::LayoutObject>>(std::move(objs)...),
			};
		}

		static LayoutResult make(layout::PageCursor cursor, std::vector<std::unique_ptr<layout::LayoutObject>> objs)
		{
			return LayoutResult {
				.cursor = std::move(cursor),
				.objects = std::move(objs),
			};
		}
	};

	struct InlineObject : Stylable
	{
		virtual ~InlineObject() = 0;
		virtual Size2d size() const { return m_size; }

	protected:
		Size2d m_size { 0, 0 };
	};

	struct BlockObject : Stylable
	{
		virtual ~BlockObject() = 0;

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const = 0;
		virtual ErrorOr<LayoutResult> createLayoutObject(interp::Interpreter* cs,
		    layout::PageCursor cursor,
		    const Style* parent_style) const = 0;

		Size2d raw_size() const { return m_size; }
		virtual Size2d size(const layout::PageCursor&) const { return m_size; }

	protected:
		Size2d m_size { 0, 0 };
	};

	/*
	    Since inline objects are not document objects, we do not have a diamond problem by
	    inheriting both of them. We want this hierarchy because script blocks and calls can
	    appear both at the top-level (with blocks or floats), and within blocks/floats as well.
	*/
	struct ScriptObject : BlockObject
	{
		explicit ScriptObject(ProcessingPhase phase) : m_run_phase(phase) { }
		ProcessingPhase runPhase() const { return m_run_phase; }

	protected:
		ProcessingPhase m_run_phase = ProcessingPhase::Layout;
	};

	struct ScriptBlock : ScriptObject
	{
		explicit ScriptBlock(ProcessingPhase phase);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		virtual ErrorOr<LayoutResult> createLayoutObject(interp::Interpreter* cs,
		    layout::PageCursor cursor,
		    const Style* parent_style) const override;

		std::unique_ptr<interp::Block> body;
	};

	struct ScriptCall : InlineObject, ScriptObject
	{
		explicit ScriptCall(ProcessingPhase phase);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		virtual ErrorOr<LayoutResult> createLayoutObject(interp::Interpreter* cs,
		    layout::PageCursor cursor,
		    const Style* parent_style) const override;

		std::unique_ptr<interp::FunctionCall> call;

	private:
		ErrorOr<std::optional<layout::PageCursor>> evaluate_scripts(interp::Interpreter* cs) const;
	};
}
