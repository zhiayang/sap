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
		std::optional<std::unique_ptr<layout::LayoutObject>> object;

		static LayoutResult empty()
		{
			return LayoutResult {
				.object = std::nullopt,
			};
		}

		static LayoutResult make(std::unique_ptr<layout::LayoutObject> obj)
		{
			return LayoutResult {
				.object = std::move(obj),
			};
		}
	};

	struct InlineObject : Stylable
	{
		virtual ~InlineObject() = 0;

		std::optional<layout::LayoutObject*> getGeneratedLayoutObject() const { return m_generated_layout_object; }

		void overrideWidth(Length length) { m_override_width = length; }
		std::optional<Length> getOverriddenWidth() const { return m_override_width; }

	protected:
		mutable std::optional<layout::LayoutObject*> m_generated_layout_object = nullptr;

	private:
		std::optional<Length> m_override_width {};
	};

	struct BlockObject : Stylable
	{
		virtual ~BlockObject() = 0;

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const = 0;
		virtual ErrorOr<LayoutResult> createLayoutObject(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const final;

		std::optional<layout::LayoutObject*> getGeneratedLayoutObject() const { return m_generated_layout_object; }

		void overrideLayoutSizeX(Length x);
		void overrideLayoutSizeY(Length y);

		void offsetRelativePosition(Size2d offset);
		void overrideAbsolutePosition(layout::AbsolutePagePos pos);

	protected:
		mutable std::optional<layout::LayoutObject*> m_generated_layout_object = nullptr;

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const = 0;

	private:
		std::optional<Length> m_override_size_x {};
		std::optional<Length> m_override_size_y {};
		std::optional<Size2d> m_rel_position_offset {};
		std::optional<layout::AbsolutePagePos> m_abs_position_override {};
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

		std::unique_ptr<interp::Block> body;

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const override;
	};

	struct ScriptCall : InlineObject, ScriptObject
	{
		explicit ScriptCall(ProcessingPhase phase);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;

		std::unique_ptr<interp::FunctionCall> call;

	private:
		using ScriptEvalResult = Either<std::vector<std::unique_ptr<InlineObject>>,
			std::unique_ptr<layout::LayoutObject>>;
		ErrorOr<std::optional<ScriptEvalResult>> evaluate_script(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const;

		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const override;

		// befriend Paragraph so it can use our evaluate_script
		friend struct Paragraph;

	private:
		mutable std::vector<std::unique_ptr<BlockObject>> m_created_tbos;
	};
}
