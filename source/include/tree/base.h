// base.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/units.h"
#include "sap/style.h"
#include "sap/annotation.h"

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

	struct InlineObject;
	struct BlockObject;

	using LinkDestination = std::variant<std::monostate, //
	    layout::AbsolutePagePos,
	    InlineObject*,
	    BlockObject*,
	    layout::LayoutObject*>;

	struct LayoutResult
	{
		std::optional<std::unique_ptr<layout::LayoutObject>> object;

		// fucking hate c++
		LayoutResult(LayoutResult&&) = default;
		LayoutResult& operator=(LayoutResult&&) = default;

		~LayoutResult();

		static LayoutResult empty();
		static LayoutResult make(std::unique_ptr<layout::LayoutObject> obj);

	private:
		LayoutResult(decltype(object) x);
	};

	struct InlineObject : zst::IntrusiveRefCounted<InlineObject>, Stylable
	{
		virtual ~InlineObject() = 0;

		void setGeneratedLayoutObject(layout::LayoutObject* obj) const { m_generated_layout_object = obj; }
		std::optional<layout::LayoutObject*> getGeneratedLayoutObject() const { return m_generated_layout_object; }

		Length raiseHeight() const { return m_raise_height; }
		void setRaiseHeight(Length raise) { m_raise_height = raise; }
		void addRaiseHeight(Length raise) { m_raise_height += raise; }

		void setLinkDestination(LinkDestination dest) { m_link_destination = std::move(dest); }

	protected:
		mutable std::optional<layout::LayoutObject*> m_generated_layout_object = nullptr;

	private:
		Length m_raise_height = 0;
		LinkDestination m_link_destination {};
	};

	struct InlineSpan : InlineObject
	{
		InlineSpan();
		explicit InlineSpan(std::vector<zst::SharedPtr<InlineObject>> objs);

		void addObject(zst::SharedPtr<InlineObject> obj);
		void addObjects(std::vector<zst::SharedPtr<InlineObject>> objs);

		const std::vector<zst::SharedPtr<InlineObject>>& objects() const { return m_objects; }
		std::vector<zst::SharedPtr<InlineObject>>& objects() { return m_objects; }

		std::vector<zst::SharedPtr<InlineObject>> flatten() &&;

		void overrideWidth(Length length) { m_override_width = length; }
		bool hasOverriddenWidth() const { return m_override_width.has_value(); }
		std::optional<Length> getOverriddenWidth() const { return m_override_width; }

	private:
		std::vector<zst::SharedPtr<InlineObject>> m_objects;
		std::optional<Length> m_override_width {};
	};

	struct BlockObject : zst::IntrusiveRefCounted<BlockObject>, Stylable
	{
		virtual ~BlockObject() = 0;

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const = 0;
		virtual ErrorOr<LayoutResult> createLayoutObject(interp::Interpreter* cs,
		    const Style* parent_style,
		    Size2d available_space) const final;

		std::optional<layout::LayoutObject*> getGeneratedLayoutObject() const { return m_generated_layout_object; }

		void overrideLayoutWidth(Length x);
		void overrideLayoutHeight(Length y);

		void offsetRelativePosition(Size2d offset);
		void overrideAbsolutePosition(layout::AbsolutePagePos pos);

		void setLinkDestination(LinkDestination dest);

	protected:
		mutable std::optional<layout::LayoutObject*> m_generated_layout_object = nullptr;

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
		    const Style* parent_style,
		    Size2d available_space) const = 0;

	private:
		std::optional<Length> m_override_width {};
		std::optional<Length> m_override_height {};
		std::optional<Size2d> m_rel_position_offset {};
		std::optional<layout::AbsolutePagePos> m_abs_position_override {};

		LinkDestination m_link_destination {};
	};

	/*
	    Since inline objects are not document objects, we do not have a diamond problem by
	    inheriting both of them. We want this hierarchy because script blocks and calls can
	    appear both at the top-level (with blocks or floats), and within blocks/floats as well.
	*/
	struct ScriptObject
	{
		explicit ScriptObject(ProcessingPhase phase) : m_run_phase(phase) { }
		ProcessingPhase runPhase() const { return m_run_phase; }

	protected:
		ProcessingPhase m_run_phase = ProcessingPhase::Layout;
	};

	struct ScriptBlock : BlockObject, ScriptObject
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

		ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const;

		std::unique_ptr<interp::FunctionCall> call;

	private:
		using ScriptEvalResult = Either<zst::SharedPtr<InlineSpan>, std::unique_ptr<layout::LayoutObject>>;
		ErrorOr<std::optional<ScriptEvalResult>> evaluate_script(interp::Interpreter* cs,
		    const Style* parent_style,
		    Size2d available_space) const;

		ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
		    const Style* parent_style,
		    Size2d available_space) const;

		// befriend Paragraph so it can use our evaluate_script
		friend struct Paragraph;

	private:
		mutable std::vector<zst::SharedPtr<BlockObject>> m_created_tbos;
	};
}
